cmake_minimum_required(VERSION 3.26)
project(unitserverFrameworkTest)

list(APPEND CMAKE_PREFIX_PATH "/Users/kirillzhukov/Documents/TS/webserver/build/UnitLib")

set(CMAKE_CXX_STANDARD 20)

find_package(UnitServerLibrary REQUIRED)
find_package(Boost REQUIRED COMPONENTS json log url)
find_package(OpenSSL REQUIRED)

add_executable(unitserverFrameworkTest main.cpp)

message(STATUS ${UnitServerLibrary_shared_LIBRARY})

target_link_libraries(${PROJECT_NAME}
        Boost::json
        Boost::log
        Boost::url
        UnitServerLibraryTargets::UnitServerLibrary_shared
        OpenSSL::SSL
)