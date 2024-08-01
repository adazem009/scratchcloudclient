// SPDX-License-Identifier: MIT

#include <iostream>

#include "cloudclient.h"
#include "cloudclient_p.h"
#include "cloudconnection.h"

using namespace scratchcloud;

/*! Constructs CloudClient and connects to a project using Scratch username, password and project ID. */
CloudClient::CloudClient(const std::string &username, const std::string &password, const std::string &projectId, int connections) :
    impl(spimpl::make_unique_impl<CloudClientPrivate>(username, password, projectId, connections))
{
}

/*! Returns true if login was successful. */
bool CloudClient::loginSuccessful() const
{
    return impl->loginSuccessful;
}

/*! Returns true if the client is connected to the project. */
bool CloudClient::connected() const
{
    return impl->connected;
}

/*! Returns the value of the given cloud variable. */
const std::string &CloudClient::getVariable(const std::string &name) const
{
    auto it = impl->variables.find(name);

    if (it == impl->variables.cend()) {
        std::cerr << "variable " << name << " not found in project" << std::endl;
        static const std::string empty;
        return empty;
    }

    return it->second;
}

/*! Sets the value of the given cloud variable. */
void CloudClient::setVariable(const std::string &name, const std::string &value)
{
    auto it = impl->variables.find(name);

    if (it == impl->variables.cend())
        std::cout << "variable " << name << " not found in project, but setting anyway" << std::endl;

    impl->variables[name] = value;
    impl->uploadVar(name, value);
}

/*! Sleeps until all variables in the queue are uploaded. */
void CloudClient::waitForUpload()
{
    while (true) {
        bool found = false;

        for (auto conn : impl->connections) {
            if (conn->queueSize() > 0) {
                found = true;
                break;
            }
        }

        if (!found)
            return;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

/*! Emits when a variable was set by another user. */
sigslot::signal<const std::string &, const std::string &> &CloudClient::variableSet()
{
    return impl->variableSet;
}
