// SPDX-License-Identifier: MIT

#pragma once

#include <string>

#include "scratchcloudclient_global.h"
#include "signal.h"
#include "spimpl.h"

namespace scratchcloud
{

class CloudClientPrivate;

/*! \brief The CloudClient class provides a simple API for Scratch cloud data. */
class SCRATCHCLOUDCLIENT_EXPORT CloudClient
{
    public:
        CloudClient(const std::string &username, const std::string &password, const std::string &projectId);
        CloudClient(const CloudClient &) = delete;

        bool loginSuccessful() const;
        bool connected() const;

        const std::string &getVariable(const std::string &name) const;
        void setVariable(const std::string &name, const std::string &value);
        void waitForUpload();

        sigslot::signal<const std::string &, const std::string &> &variableSet();

    private:
        spimpl::unique_impl_ptr<CloudClientPrivate> impl;
};

} // namespace scratchcloud
