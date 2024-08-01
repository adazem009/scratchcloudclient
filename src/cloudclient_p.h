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
        CloudClientPrivate(const std::string &username, const std::string &password, const std::string &projectId, int connections);
        CloudClientPrivate(const CloudClientPrivate &) = delete;

        void login();

        void uploadVar(const std::string &name, const std::string &value);
        void processEvent(const std::string &name, const std::string &value);

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
        sigslot::signal<const std::string &, const std::string &> variableSet;
};

} // namespace scratchcloud
