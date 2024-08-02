#include <iostream>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

#include "cloudconnection.h"

#define MAX_ATTEMPTS 32
#define UPLOAD_WAIT_TIME 150
#define CONNECTION_TIMEOUT 5000
#define RESPONSE_TIMEOUT 5000

using namespace scratchcloud;

CloudConnection::CloudConnection(int id, const std::string &username, const std::string &sessionId, const std::string &projectId) :
    m_id(id),
    m_username(username),
    m_sessionId(sessionId),
    m_projectId(projectId)
{
    m_url = "wss://clouddata.scratch.mit.edu";
    connect();

    m_loopThread = std::thread([&]() { uploadLoop(); });
}

CloudConnection::~CloudConnection()
{
    // Disconnect
    m_stopLoop = true;
    std::cout << m_id << ": disconnecting..." << std::endl;

    if (m_loopThread.joinable())
        m_loopThread.join();
}

bool CloudConnection::connected() const
{
    return m_connected;
}

int CloudConnection::queueSize() const
{
    m_uploadMutex.lock();
    int size = m_uploadQueue.size();
    m_uploadMutex.unlock();

    return size;
}

void CloudConnection::uploadVar(const std::string &name, const std::string &value)
{
    m_uploadMutex.lock();
    m_uploadQueue.push_back({ name, value });
    m_uploadMutex.unlock();
}

sigslot::signal<const std::string &, const std::string &> &CloudConnection::variableSet() const
{
    return m_variableSet;
}

void CloudConnection::connect()
{
    if (m_attempt >= MAX_ATTEMPTS) {
        std::cerr << m_id << ": failed to connect after " << m_attempt << " attempts, you should restart your server program now" << std::endl;
        return;
    }

    m_attempt++;
    assert(m_attempt <= MAX_ATTEMPTS);
    std::cout << m_id << ": connecting to " << m_url << " (attempt " << m_attempt << " of " << MAX_ATTEMPTS << ")" << std::endl;
    m_responseReceived = false;
    m_reconnect = false;
    m_websocket = std::make_shared<ix::WebSocket>();
    m_websocket->setUrl(m_url);

    // Message callback
    m_websocket->setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        switch (msg->type) {
            case ix::WebSocketMessageType::Close:
                // Connection lost
                if (m_connected) {
                    std::cout << m_id << ": connection lost!" << std::endl;
                    m_connected = false;
                    m_reconnect = true;
                    break;
                }

            case ix::WebSocketMessageType::Message: {
                // Message received
                m_responseReceived = true;

                if (m_ignoreNextMessage) {
                    m_ignoreNextMessage = false;
                    return;
                }

                std::vector<std::string> response = splitStr(msg->str, "\n");

                for (int i = 0; i < response.size() - 1; i++) {
                    response[i].erase(response[i].find(u8"☁ "), 4);

                    try {
                        nlohmann::json json = nlohmann::json::parse(response[i]);
                        std::string name = json["name"];
                        std::string value;
                        nlohmann::json jsonValue = json["value"];

                        if (jsonValue.is_number())
                            value = jsonValue.dump();
                        else
                            value = json["value"];

                        m_variableSet(name, value);
                    } catch (std::exception &e) {
                        std::cerr << "invalid message JSON: " << response[i] << std::endl;
                        std::cerr << e.what() << std::endl;
                    }
                }
                break;
            }

            default:
                break;
        }
    });

    // Connect
    ix::WebSocketHttpHeaders extra_headers;
    extra_headers["cookie"] = "scratchsessionsid=" + m_sessionId + ";";
    extra_headers["origin"] = "https://scratch.mit.edu";
    extra_headers["enable_multithread"] = true;
    m_websocket->setExtraHeaders(extra_headers);
    m_websocket->disableAutomaticReconnection();

    ix::WebSocketInitResult result = m_websocket->connect(CONNECTION_TIMEOUT / 1000);

    if (!result.success) {
        // Failure
        std::cerr << m_id << ": failed to connect: " << result.errorStr;
        std::cerr << m_id << ": reconnecting..." << std::endl;
        connect();
        return;
    }

    // Handshake
    m_websocket->start();
    m_websocket->send("{\"method\":\"handshake\", \"user\":\"" + m_username + "\", \"project_id\":\"" + m_projectId + "\" }\n");

    // Wait for response with variable list
    TimePoint start = std::chrono::steady_clock::now();

    while (!m_responseReceived) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (delta > RESPONSE_TIMEOUT) {
            std::cout << m_id << ": didn't receive response, reconnecting..." << std::endl;
            m_websocket->close();
            connect();
            return;
        }
    }

    m_attempt = 0;
    m_connected = true;
}

void CloudConnection::uploadLoop()
{
    // Runs in another thread to send messages with a delay
    while (!m_stopLoop) {
        if (m_reconnect) {
            // Since we're reconnecting, we don't need to read the list of variables again
            m_ignoreNextMessage = true;
            std::cout << m_id << ": reconnecting..." << std::endl;
            m_attempt = 0;
            m_websocket->close();
            connect();
        }

        if (m_connected) {
            m_uploadMutex.lock();

            if (!m_uploadQueue.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpload).count();

                if (delta >= UPLOAD_WAIT_TIME) {
                    // Send queued message
                    const auto &pair = m_uploadQueue.front();

                    const auto &name = pair.first;
                    const auto &value = pair.second;
                    m_websocket->send(u8"{ \"method\":\"set\", \"name\":\"☁ " + name + "\", \"value\":\"" + value + "\", \"user\":\"" + m_username + "\", \"project_id\":\"" + m_projectId + "\" }\n");
                    m_uploadQueue.erase(m_uploadQueue.begin());
                    m_lastUpload = now;
                }
            }

            m_uploadMutex.unlock();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

std::vector<std::string> CloudConnection::splitStr(const std::string &str, const std::string &separator)
{
    int start = 0;
    int end = str.find(separator);
    std::vector<std::string> output;

    while (end != -1) {
        output.emplace_back(str.substr(start, end - start));
        start = end + separator.size();
        end = str.find(separator, start);
    }

    output.emplace_back(str.substr(start, end - start));
    return output;
}
