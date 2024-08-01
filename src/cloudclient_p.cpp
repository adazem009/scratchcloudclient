// SPDX-License-Identifier: MIT

#include <iostream>
#include <regex>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "cloudclient_p.h"

#define UPLOAD_WAIT_TIME 150

using namespace scratchcloud;

CloudClientPrivate::CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId) :
    username(username),
    password(password),
    projectId(projectId)
{
    login();

    if (!loginSuccessful)
        return;

    connectionThread = std::thread([&]() { connect(); });
    connectionThread.join();

    connectionThread = std::thread([&]() { uploadLoop(); });
}

CloudClientPrivate::~CloudClientPrivate()
{
    stopConnectionThread = true;

    if (connectionThread.joinable())
        connectionThread.join();
}

void CloudClientPrivate::login()
{
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
            loginAttempts++;

            if (loginAttempts < 25)
                login();

            return;
        }
    }

    std::smatch login_smatch;
    std::regex_search(login_response.raw_header, login_smatch, std::regex("\"(.*)\""));

    sessionId = login_smatch[0];
    xToken = nlohmann::json::parse(login_response.text)[0]["token"];
    loginSuccessful = true;
    loginAttempts = 0;
    return;
}

void CloudClientPrivate::connect()
{
    connected = false;

    if (!loginSuccessful)
        return;

    reconnect = false;

    websocket = std::make_unique<ix::WebSocket>();
    websocket->setUrl("wss://clouddata.scratch.mit.edu/");

    websocket->setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        if (reconnect)
            return;

        switch (msg->type) {
            case ix::WebSocketMessageType::Close:
                reconnect = true;
                break;

            case ix::WebSocketMessageType::Message: {
                if (ignoreMessages) {
                    ignoreMessages = false;
                    return;
                }

                std::vector<std::string> response = splitStr(msg->str, "\n");

                for (int i = 0; i < response.size() - 1; i++) {
                    response[i].erase(response[i].find(u8"☁ "), 4);
                    nlohmann::json json = nlohmann::json::parse(response[i]);
                    std::string name = json["name"];
                    std::string value = json["value"];
                    variables[name] = value;
                    variableSet(name, value);
                }
                break;
            }

            default:
                break;
        }
    });

    ix::WebSocketHttpHeaders extra_headers;
    extra_headers["cookie"] = "scratchsessionsid=" + sessionId + ";";
    extra_headers["origin"] = "https://scratch.mit.edu";
    extra_headers["enable_multithread"] = true;
    websocket->setExtraHeaders(extra_headers);
    websocket->disableAutomaticReconnection();

    ix::WebSocketInitResult result = websocket->connect(5);

    if (!result.success) {
        std::cout << "failed to connect: " << result.errorStr << std::endl;
        return;
    }

    websocket->start();
    websocket->send("{\"method\":\"handshake\", \"user\":\"" + username + "\", \"project_id\":\"" + projectId + "\" }\n");

    connected = true;

    if (ignoreMessages)
        return;

    int i = 0;

    while (variables.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        i++;

        if (i > 25)
            return;
    }

    return;
}

void CloudClientPrivate::uploadLoop()
{
    while (!stopConnectionThread) {
        if (reconnect) {
            ignoreMessages = true;
            websocket->close();
            websocket.reset();
            login();
            connect();
        }

        uploadMutex.lock();

        for (auto &[name, info] : uploadQueue) {
            auto lastUpload = info.first;
            auto &values = info.second;
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpload).count();

            if (delta >= UPLOAD_WAIT_TIME && !values.empty()) {
                const std::string &value = values.front();
                websocket->send(u8"{ \"method\":\"set\", \"name\":\"☁ " + name + "\", \"value\":\"" + value + "\", \"user\":\"" + username + "\", \"project_id\":\"" + projectId + "\" }\n");
                values.erase(values.begin());
                info.first = now;
            }
        }

        uploadMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void CloudClientPrivate::uploadVar(const std::string &name, const std::string &value)
{
    uploadMutex.lock();

    if (uploadQueue.find(name) == uploadQueue.cend())
        uploadQueue[name] = { TimePoint(), {} };

    uploadQueue[name].second.push_back(value);

    uploadMutex.unlock();
}

std::vector<std::string> CloudClientPrivate::splitStr(const std::string &str, const std::string &separator)
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
