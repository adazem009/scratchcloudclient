# Scratch cloud client

A simple and easy to use C++ Scratch cloud data client.
Based on [Cpp-ScratchClient](https://github.com/nini2009ph/Cpp-ScratchClient) by [nini2009ph](https://github.com/nini2009ph).

# Using in a CMake project
Using FetchContent:
```cmake
include(FetchContent)
FetchContent_Declare(cloudclient GIT_REPOSITORY https://github.com/adazem009/scratchcloudclient.git GIT_TAG f1395abbd26b5dd308a83f4296f50700ee4256f5)
FetchContent_MakeAvailable(cloudclient)
target_link_libraries(MyApp PRIVATE scratchcloudclient)
```

Or if you cloned the repository (submodule, etc.):
```cmake
add_subdirectory(scratchcloudclient)
target_link_libraries(MyApp PRIVATE scratchcloudclient)
```

# Example
```cpp
#include <scratchcloudclient/cloudclient.h>
#include <iostream>

using namespace scratchcloud;

int main() {
    CloudClient client("username", "password", "526557379");

    if(!client.loginSuccessful() || !client.connected())
        return 1;

    // Set variable
    client.setVariable("var1", "10");
    client.setVariable("var2", "8");
    client.setVariable("var3", "5000");

    // Read variable
    std::cout << client.getVariable("var2") << std::endl;

    // Event (when a variable is changed by another user)
    client.variableSet().connect([](const CloudEvent &event) {
        std::cout << event.user() << ": " << event.name() << " = " << event.value() << std::endl;
    });

    getc(stdin); // press enter/return to stop
    return 0;
}
```
Please note that it's fine to call `setVariable()` multiple times without any delay because uploading happens in another thread
where the messages are sent with a delay to avoid data loss.

To be able to upload multiple variables simultaneously, multiple connections are used.
You can pass the amount of them to the constructor. The default is **10**.
```cpp
CloudClient client("username", "password", "526557379", 4);
```

# Listen modes
There are 2 listen modes: **CloudLog** and **Websockets**
The default is **CloudLog** which is based on fetching cloud logs using Scratch API.
The advantage of this mode is that it allows you to read who set the cloud variable.
However, it's pretty slow compared to the **Websockets** mode which is real time.

The **WebSockets** mode is based on listening to Websockets messages just like
a regular Scratch client does. It's good for multiplayer games, but it doesn't
support reading who set the cloud variable.

You will probably need both modes in advanced projects. Because of that, it's possible
to set different mode for each variable.
