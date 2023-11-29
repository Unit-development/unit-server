//
// Created by Kirill Zhukov on 15.11.2023.
//

#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <functional>
#include <thread>
#include "types.h"
#include "HandlerModule.h"

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

namespace unit::server {
    namespace data {
        class HttpRequest;
        class HttpResponse;
    }; // data

    namespace regex::basic {
        class BasicEndpointHandler;
    }; // regex::basic

    namespace configuration {
        class ConfigReader;
    }; // configuration

    namespace handler {
        class UNITSERVER_API Server {
        public:
            explicit Server(const std::string&config_path);

            void handle(request::type request_type, const std::string&endpoint,
                        const std::function<void (data::HttpRequest&, data::HttpResponse&)>& function);

            void addModule(module::HandlerModule &module);

            [[noreturn]] void start();

        public:
            std::shared_ptr<configuration::ConfigReader> config;

        private:
            std::vector<std::thread> iocs;
            std::shared_ptr<regex::basic::BasicEndpointHandler> endpoint_handler;
        };
    }; // handler
}; // unit::server


#endif //SERVER_H
