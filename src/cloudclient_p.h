// SPDX-License-Identifier: MIT

#pragma once

#include <unordered_map>
#include <string>
#include <set>

#include "signal.h"

namespace scratchcloud
{

class CloudConnection;

struct CloudClientPrivate
{
        using TimePoint = std::chrono::steady_clock::time_point;

        CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId, int connections);
        CloudClientPrivate(const CloudClientPrivate &) = delete;
        ~CloudClientPrivate();

        void login();

        void uploadVar(const std::string &name, const std::string &value);
        void listenToMessages();
        void processEvent(CloudConnection *connection, const std::string &name, const std::string &value);

        std::string username;
        std::string password;
        std::string sessionId;
        std::string xToken;
        std::string projectId;
        int attempt = 0;
        bool loginSuccessful = false;
        bool connected = false;
        std::set<std::shared_ptr<CloudConnection>> connections;
        std::unordered_map<std::string, std::string> variables;
        std::unordered_map<CloudConnection *, std::vector<std::pair<std::string, std::string>>> receivedMessages;
        TimePoint listenStartTime;
        std::atomic<bool> listening = false;
        std::thread listenThread;
        std::mutex listenMutex;
        std::atomic<bool> stopListenThread = false;
        sigslot::signal<const std::string &, const std::string &> variableSet;
};

} // namespace scratchcloud
