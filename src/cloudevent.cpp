// SPDX-License-Identifier: MIT

#include <iostream>

#include "cloudevent.h"
#include "cloudevent_p.h"

using namespace scratchcloud;

CloudEvent::CloudEvent(CloudClient::ListenMode listenMode, const std::string &user, const std::string &name, const std::string &value) :
    impl(spimpl::make_impl<CloudEventPrivate>(listenMode, user, name, value))
{
}

const std::string &CloudEvent::user() const
{
    if (impl->listenMode == CloudClient::ListenMode::Websockets) {
        std::cerr << "Websockets mode doesn't support reading setter username! Use the CloudLog mode instead." << std::endl;
        static const std::string empty;
        return empty;
    } else
        return impl->user;
}

const std::string &CloudEvent::name() const
{
    return impl->name;
}

const std::string &CloudEvent::value() const
{
    return impl->value;
}
