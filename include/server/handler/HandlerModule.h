//
// Created by Kirill Zhukov on 29.11.2023.
//

#ifndef HANDLERMODULE_H
#define HANDLERMODULE_H

#include <functional>
#include "types.h"
#include "EndpointHandler.h"

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

    namespace module {
        class UNITSERVER_API HandlerModule {
        public:
            virtual ~HandlerModule() = default;

            virtual void handle(request::type request_type, const std::string&endpoint,
                                const std::function<void (data::HttpRequest&, data::HttpResponse&)>& function) = 0;

            virtual std::shared_ptr<regex::EndpointHandler<>> getNativeHandler() = 0;
        };
    }; // module
}; // unit::server

#endif //HANDLERMODULE_H
