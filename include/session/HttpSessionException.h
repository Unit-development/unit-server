//
// Created by Kirill Zhukov on 03.11.2023.
//

#ifndef TCPSOCKETTEST_HTTPSESSIONEXCEPTION_H
#define TCPSOCKETTEST_HTTPSESSIONEXCEPTION_H

#include <exception>
#include <string>
#include <utility>

#ifndef UNITSERVER_API
#ifdef _WIN32
#ifdef BUILDING_UNITSERVER
#define UNITSERVER_API __declspec(dllexport)
#else
#define UNITSERVER_API __declspec(dllimport)
#endif
#else
#define UNITSERVER_API __attribute__((visibility("default")))
#endif
#endif

class UNITSERVER_API StreamNotFound final : public std::exception {
public:
    // Constructor (C++11 onward)
    explicit StreamNotFound(std::string msg) : message(std::move(msg)) {}

    // Override the what() method from the base class
    [[nodiscard]] const char *what() const noexcept override;
private:
    std::string message;
};

#endif //TCPSOCKETTEST_HTTPSESSIONEXCEPTION_H
