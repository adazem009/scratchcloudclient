// SPDX-License-Identifier: MIT

#pragma once

#include "cloudclient.h"

namespace scratchcloud
{

class CloudEventPrivate;

class CloudEvent
{
    public:
        CloudEvent(CloudClient::ListenMode listenMode, const std::string &user, const std::string &name, const std::string &value);

        const std::string &user() const;
        const std::string &name() const;
        const std::string &value() const;

    private:
        spimpl::impl_ptr<CloudEventPrivate> impl;
};

} // namespace scratchcloud
