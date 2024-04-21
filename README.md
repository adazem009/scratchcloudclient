# Scratch cloud client

A simple and easy to use C++ Scratch cloud data client.
Based on [Cpp-ScratchClient](https://github.com/nini2009ph/Cpp-ScratchClient) by [nini2009ph](https://github.com/nini2009ph).

# Using in a CMake project
```cmake
add_subdirectory(scratchcloudclient)
target_include_directories(MyApp PRIVATE scratchcloudclient/include)
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
    client.variableSet().connect([](const std::string& name, const std::string& value) {
        std::cout << name << " = " << value << std::endl;
    });

    getc(stdin); // press enter/return to stop
    return 0;
}
```
Please note that it's fine to call `setVariable()` multiple times without any delay because uploading happens in another thread
where the messages are sent with a delay to avoid data loss.
