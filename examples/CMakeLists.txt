cmake_minimum_required(VERSION 3.26)
project(unitserverFrameworkTest)

list(APPEND CMAKE_PREFIX_PATH "<build_path_for_unit_server>")

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
        Unit::Server_shared
        OpenSSL::SSL
)