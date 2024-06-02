// SPDX-License-Identifier: MIT

#pragma once

#include <unordered_map>
#include <string>
#include <ixwebsocket/IXWebSocket.h>

#include "signal.h"

namespace scratchcloud
{

struct CloudClientPrivate
{
        CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId);
        CloudClientPrivate(const CloudClientPrivate &) = delete;
        ~CloudClientPrivate();

        void login();
        std::string getCsrfToken() const;

        void connect();
        void uploadLoop();
        void uploadVar(const std::string &name, const std::string &value);

        static std::vector<std::string> splitStr(const std::string &str, const std::string &separator);

        std::string username;
        std::string password;
        std::string sessionId;
        std::string xToken;
        std::string csrfToken;
        std::string projectId;
        int loginAttempts = 0;
        bool loginSuccessful = false;
        bool connected = false;
        std::unique_ptr<ix::WebSocket> websocket;
        std::thread connectionThread;
        std::chrono::steady_clock::time_point lastUpload;
        std::atomic<bool> stopConnectionThread = false;
        std::atomic<bool> reconnect = false;
        std::atomic<bool> ignoreMessages = false;
        std::mutex uploadMutex;
        std::vector<std::pair<std::string, std::string>> uploadQueue;
        std::unordered_map<std::string, std::string> variables;
        sigslot::signal<const std::string &, const std::string &> variableSet;
};

} // namespace scratchcloud
