// SPDX-License-Identifier: MIT

#include <iostream>
#include <regex>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "cloudclient_p.h"
#include "cloudconnection.h"

#define MAX_LOGIN_ATTEMPTS 32
#define LISTEN_TIME 50

using namespace scratchcloud;

CloudClientPrivate::CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId, int connections) :
    username(username),
    password(password),
    projectId(projectId)
{
    login();

    if (!loginSuccessful)
        return;

    // Create connections
    const int threadCount = std::thread::hardware_concurrency();
    std::mutex connectionMutex;

    auto f = [this, &connectionMutex, &username, &projectId](int id) {
        auto conn = std::make_shared<CloudConnection>(id, username, sessionId, projectId);
        conn->variableSet().connect([conn, this](const std::string &name, const std::string &value) { processEvent(conn.get(), name, value); });

        connectionMutex.lock();
        this->connections.insert(conn);
        connectionMutex.unlock();
    };

    std::vector<std::thread> threads;
    bool done = false;
    int i = 0;

    while (true) {
        threads.clear();

        for (int j = 0; j < threadCount; j++) {
            if (i >= connections) {
                done = true;
                break;
            }

            threads.push_back(std::thread(f, i));
            i++;
        }

        for (int i = 0; i < threads.size(); i++)
            threads[i].join();

        if (done)
            break;
    }

    // Check connection status
    for (auto conn : this->connections) {
        if (!conn->connected())
            return;

        receivedMessages[conn.get()] = {};
    }

    listenThread = std::thread([&]() { listenToMessages(); });
    connected = true;
}

CloudClientPrivate::~CloudClientPrivate()
{
    stopListenThread = true;

    if (listenThread.joinable())
        listenThread.join();
}

void CloudClientPrivate::login()
{
    if (attempt >= MAX_LOGIN_ATTEMPTS) {
        std::cerr << "failed to log in after " << attempt << " attempts!" << std::endl;
        return;
    }

    attempt++;
    assert(attempt <= MAX_LOGIN_ATTEMPTS);
    std::cout << "attempting to log in... (attempt " << attempt << " of " << MAX_LOGIN_ATTEMPTS << ")" << std::endl;
    loginSuccessful = false;
    cpr::Url login_url{ "https://scratch.mit.edu/login/" };
    cpr::Header login_headers{
        { "x-csrftoken", "a" },
        { "x-requested-with", "XMLHttpRequest" },
        { "Cookie", "scratchcsrftoken=a;scratchlanguage=en;" },
        { "referer", "https://scratch.mit.edu" },
        { "user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.101 Safari/537.36" }
    };
    cpr::Body login_body{ "{ \"username\": \"" + username + "\", \"password\": \"" + password + "\" }" };
    cpr::Response login_response = cpr::Post(login_url, login_headers, login_body);

    if (login_response.status_code != 200) {
        if (login_response.status_code == 403) {
            std::cerr << "Incorrect username or password!" << std::endl;
            return;
        } else {
            login();
            return;
        }
    }

    std::smatch login_smatch;
    std::regex_search(login_response.raw_header, login_smatch, std::regex("\"(.*)\""));

    sessionId = login_smatch[0];
    xToken = nlohmann::json::parse(login_response.text)[0]["token"];
    loginSuccessful = true;
    attempt = 0;
    std::cout << "success!" << std::endl;
    return;
}

void CloudClientPrivate::uploadVar(const std::string &name, const std::string &value)
{
    // Pick the least overloaded connection
    int min = 0;
    std::shared_ptr<CloudConnection> conn = nullptr;

    for (auto c : connections) {
        if (!conn || c->queueSize() < min) {
            conn = c;
            min = c->queueSize();
        }
    }

    if (conn)
        conn->uploadVar(name, value);
}

void CloudClientPrivate::listenToMessages()
{
    /*
     * Since we're using multiple connections, messages sent by this program
     * cannot be filtered properly. Because of this, we need to listen to
     * messages for some time and then determine which messages should be
     * filtered (messages sent by a client are not returned to it).
     */
    while (!stopListenThread) {
        listenMutex.lock();

        if (listening) {
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - listenStartTime).count();

            if (delta >= LISTEN_TIME) {
                // Create a list of distinct messages
                std::vector<std::pair<std::string, std::string>> distinctMessages;

                for (const auto &[conn, list] : receivedMessages) {
                    for (const auto &message : list) {
                        if (std::find(distinctMessages.begin(), distinctMessages.end(), message) == distinctMessages.end())
                            distinctMessages.push_back(message);
                    }
                }

                // Notify about messages which are present in the same count in all connections
                for (const auto &message : distinctMessages) {
                    bool skip = false;
                    int count = -1;

                    for (const auto &[conn, list] : receivedMessages) {
                        int currentCount = std::count(list.begin(), list.end(), message);

                        if ((count != -1 && currentCount != count) || currentCount == 0) {
                            // This message should be skipped
                            skip = true;
                            break;
                        }

                        count = currentCount;
                    }

                    if (!skip) {
                        variables[message.first] = message.second;
                        variableSet(message.first, message.second);
                    }
                }

                // Clear received messages
                for (auto &[conn, list] : receivedMessages)
                    list.clear();

                listening = false;
            }
        }

        listenMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

void CloudClientPrivate::processEvent(CloudConnection *connection, const std::string &name, const std::string &value)
{
    listenMutex.lock();

    if (!listening) {
        listening = true;
        listenStartTime = std::chrono::steady_clock::now();
    }

    receivedMessages[connection].push_back({ name, value });
    listenMutex.unlock();
}
