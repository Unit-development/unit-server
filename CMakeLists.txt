cmake_minimum_required(VERSION 3.26)
project(Server VERSION 0.0.5)

set(CMAKE_CXX_STANDARD 20)

set(BUILDING_UNITSERVER ON)

# Now, you can conditionally check its value and apply build configurations as needed.
if(BUILDING_UNITSERVER)
    add_definitions(-DBUILDING_UNITSERVER)
    message(STATUS "BUILDING_UNITSERVER is set.")
else()
    message(STATUS "BUILDING_UNITSERVER is not set.")
endif()

find_package(Boost REQUIRED COMPONENTS json log url)
find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/include/*.hpp")

set(INCLUDE_DIRS "")
foreach(HEADER ${HEADER_FILES})
    get_filename_component(DIR ${HEADER} DIRECTORY)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${DIR})
endforeach()

list(REMOVE_DUPLICATES INCLUDE_DIRS)

include_directories(${INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_definitions(
        -DDEBUG=0
        -DDEBUG_ON_RECEIVE=0
        -DDEBUG_HEADERS=0
        -DDEBUG_SSL_HANDSHAKE=0
        -DDEBUG_ON_FRAME=0
        -DDEBUG_FRAME_CALLBACK=0
        -DDEBUG_CHUNK_RECEIVED=0
        -DDEBUG_SOCKET_WRITING=0
        -DDEBUG_SOCKET_WRITING_ERROR=1
        -DDEBUG_RESPONSE_EMULATION=0
        -DDEBUG_STREAM_PRINTING=0
        -DDEBUG_CONNECTION=0
        -DSSL_DEBUG=0
)

add_library(${PROJECT_NAME}_shared SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

add_library(${PROJECT_NAME}_static STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

# # Decide lib subfolder based on architecture
# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#     set(ARCH_SUBFOLDER "x64")
# else()
#     set(ARCH_SUBFOLDER "x86")
# endif()

# Install targets and generate export targets
install(TARGETS ${PROJECT_NAME}_shared EXPORT UnitServerTargets
    LIBRARY DESTINATION lib/${ARCH_SUBFOLDER}
    ARCHIVE DESTINATION lib/${ARCH_SUBFOLDER}
    RUNTIME DESTINATION lib/${ARCH_SUBFOLDER}
    INCLUDES DESTINATION include
)

install(TARGETS ${PROJECT_NAME}_static EXPORT UnitServerTargets
    LIBRARY DESTINATION lib/${ARCH_SUBFOLDER}
    ARCHIVE DESTINATION lib/${ARCH_SUBFOLDER}
    RUNTIME DESTINATION lib/${ARCH_SUBFOLDER}
    INCLUDES DESTINATION include
)

file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.h" "${CMAKE_SOURCE_DIR}/include/*.hpp")

list(FILTER HEADER_FILES EXCLUDE REGEX "include/internal/.*")

install(FILES ${HEADER_FILES} DESTINATION include)

foreach(HEADER_FILE ${HEADER_FILES})
    message(STATUS "Header file: ${HEADER_FILE}")
endforeach()

install(EXPORT UnitServerTargets
    FILE UnitServerTargets.cmake
    NAMESPACE Unit::
    DESTINATION cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/UnitServerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/UnitServerConfig.cmake
    INSTALL_DESTINATION cmake
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/UnitServerConfig.cmake
    DESTINATION cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/UnitServerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/UnitServerConfigVersion.cmake
    DESTINATION cmake
)

# Search for nghttp2 shared library in common directories
find_library(LIBNGHTTP2_LIBRARY 
    NAMES nghttp2
    PATHS /usr/local/lib
    NO_DEFAULT_PATH
)

# If not found, provide an error message
if(NOT LIBNGHTTP2_LIBRARY)
    message(FATAL_ERROR "libnghttp2 shared library not found")
endif()

set(UnitServer_LIBRARY ${CMAKE_CURRENT_BINARY_DIR})
set(UnitServer_INCLUDE ${CMAKE_CURRENT_INCLUDES_DIR})

# Linking against the found nghttp2 library
target_link_libraries(${PROJECT_NAME}_shared
    OpenSSL::SSL
    Boost::json
    Boost::boost
    Boost::log
    Boost::url
    ${LIBNGHTTP2_LIBRARY}
)

target_link_libraries(${PROJECT_NAME}_static
    OpenSSL::SSL
    Boost::json
    Boost::boost
    Boost::log
    Boost::url
    ${LIBNGHTTP2_LIBRARY}
)

# add_executable(tcpSocketTest main.cpp)
