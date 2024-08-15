// SPDX-License-Identifier: MIT

#pragma once

#include "cloudclient.h"

namespace scratchcloud
{

struct CloudEventPrivate
{
        CloudEventPrivate(CloudClient::ListenMode listenMode, const std::string &user, const std::string &name, const std::string &value);

        CloudClient::ListenMode listenMode = CloudClient::ListenMode::CloudLog;
        std::string user;
        std::string name;
        std::string value;
};

} // namespace scratchcloud
