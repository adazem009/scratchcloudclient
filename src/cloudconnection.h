// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>

#include "signal.h"

namespace ix
{
class WebSocket;
}

namespace scratchcloud
{

class CloudConnection
{
    public:
        CloudConnection(int id, const std::string &username, const std::string &sessionId, const std::string &projectId);
        ~CloudConnection();

        int id() const;

        bool connected() const;

        int queueSize() const;
        void uploadVar(const std::string &name, const std::string &value);
        sigslot::signal<const std::string &, const std::string &> &variableSet() const;

    private:
        using TimePoint = std::chrono::steady_clock::time_point;

        void connect();
        void uploadLoop();
        static std::vector<std::string> splitStr(const std::string &str, const std::string &separator);

        int m_id;
        std::string m_username;
        std::string m_sessionId;
        std::string m_projectId;
        std::string m_url;
        bool m_connected = false;
        std::shared_ptr<ix::WebSocket> m_websocket;
        int m_attempt = 0;
        bool m_reconnect = false;
        bool m_responseReceived = false;
        bool m_ignoreNextMessage = false;
        std::thread m_loopThread;
        bool m_stopLoop = false;
        mutable std::mutex m_uploadMutex;
        std::vector<std::pair<std::string, std::string>> m_uploadQueue;
        TimePoint m_lastUpload;
        mutable sigslot::signal<const std::string &, const std::string &> m_variableSet;
};

} // namespace scratchcloud
