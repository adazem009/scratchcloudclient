// SPDX-License-Identifier: MIT

#include "cloudevent_p.h"

using namespace scratchcloud;

CloudEventPrivate::CloudEventPrivate(CloudClient::ListenMode listenMode, const std::string &user, const std::string &name, const std::string &value) :
    listenMode(listenMode),
    user(user),
    name(name),
    value(value)
{
}
