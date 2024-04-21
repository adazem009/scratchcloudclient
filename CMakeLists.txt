cmake_minimum_required(VERSION 3.14)

project(scratchcloudclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR include/scratchcloudclient)

add_library(scratchcloudclient SHARED
  ${INCLUDE_DIR}/scratchcloudclient_global.h
  ${INCLUDE_DIR}/spimpl.h
  ${INCLUDE_DIR}/signal.h
  ${INCLUDE_DIR}/cloudclient.h
)

target_sources(scratchcloudclient
  PRIVATE
    src/cloudclient.cpp
    src/cloudclient_p.cpp
    src/cloudclient_p.h
)

target_compile_definitions(scratchcloudclient PRIVATE SCRATCHCLOUDCLIENT_LIBRARY)
target_include_directories(scratchcloudclient PRIVATE ${INCLUDE_DIR})

# cpr
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # 1.10.5
FetchContent_MakeAvailable(cpr)
target_link_libraries(scratchcloudclient PUBLIC cpr::cpr)

# ixwebsocket
FetchContent_Declare(ixwebsocket GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git GIT_TAG 75e9c84388879262d924e0582f094b38550093dc) # the commit hash for 1.5.0
FetchContent_MakeAvailable(ixwebsocket)
set(USE_TLS ON)
target_link_libraries(scratchcloudclient PUBLIC ixwebsocket)

# nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(scratchcloudclient PUBLIC nlohmann_json::nlohmann_json)
