// SPDX-License-Identifier: MIT

#pragma once

#include <unordered_map>
#include <string>
#include <set>

#include "signal.h"
#include "cloudlogrecord.h"
#include "cloudclient.h"

namespace scratchcloud
{

class CloudConnection;

struct CloudClientPrivate
{
        using TimePoint = std::chrono::steady_clock::time_point;

        CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId, int connections);
        CloudClientPrivate(const CloudClientPrivate &) = delete;
        ~CloudClientPrivate();

        void login(int attempt = 1);
        void connect();
        void reconnect();

        void uploadVar(const std::string &name, const std::string &value);
        void listenToCloudLog();
        void listenToMessages();
        void notifyAboutVar(CloudClient::ListenMode srcMode, const std::string &user, const std::string &name, const std::string &value);
        void processEvent(CloudConnection *connection, const std::string &name, const std::string &value);

        void getCloudLog(std::vector<CloudLogRecord> &out, int limit = 25, int offset = 0);

        std::string username;
        std::string password;
        std::string sessionId;
        std::string xToken;
        std::string projectId;
        int connectionCount = 0;
        bool loginSuccessful = false;
        bool connected = false;
        std::set<std::shared_ptr<CloudConnection>> connections;
        std::unordered_map<std::string, std::string> variables;
        std::unordered_map<std::string, CloudClient::ListenMode> variablesListenMode;
        CloudClient::ListenMode defaultListenMode = CloudClient::ListenMode::CloudLog;
        std::unordered_map<CloudConnection *, std::vector<std::pair<std::string, std::string>>> receivedMessages;
        long cloudLogReadTime = 0;
        TimePoint listenStartTime;
        std::atomic<bool> listening = false;
        TimePoint lastWsActivity;
        TimePoint lastUpload;
        std::thread cloudLogThread;
        std::thread wsThread;
        std::thread reconnectThread;
        std::mutex listenMutex;
        std::atomic<bool> stopListenThreads = false;
        sigslot::signal<const CloudEvent &> variableSet;
};

} // namespace scratchcloud
