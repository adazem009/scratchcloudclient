// SPDX-License-Identifier: MIT

#pragma once

#include <string>

#include "scratchcloudclient_global.h"
#include "signal.h"
#include "spimpl.h"

namespace scratchcloud
{

class CloudEvent;
class CloudClientPrivate;

/*! \brief The CloudClient class provides a simple API for Scratch cloud data. */
class SCRATCHCLOUDCLIENT_EXPORT CloudClient
{
    public:
        enum class ListenMode
        {
            CloudLog,  /*!< (Default) Uses Scratch API to read the cloud log. It's slower, but makes it possible to read the variable setter username. */
            Websockets /*!< Listens to messages using Websockets. Good for multiplayer games because it's real time, but you won't be able to read the setter username. */
        };

        CloudClient(const std::string &username, const std::string &password, const std::string &projectId, int connections = 10);
        CloudClient(const CloudClient &) = delete;

        bool loginSuccessful() const;
        bool connected() const;

        const std::string &getVariable(const std::string &name) const;
        void setVariable(const std::string &name, const std::string &value);
        void waitForUpload();

        void setListenMode(ListenMode newMode);
        void setVariableListenMode(const std::string &name, ListenMode mode);

        sigslot::signal<const CloudEvent &> &variableSet();

    private:
        spimpl::unique_impl_ptr<CloudClientPrivate> impl;
};

} // namespace scratchcloud
