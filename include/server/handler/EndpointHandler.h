//
// Based on Vadim Zhukov's code, 20.11.2023.
//

#ifndef HANDLER_H
#define HANDLER_H
#include <regex>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "types.h"

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

namespace unit::server::regex {
    template<class UserData = std::string>
    class UNITSERVER_API EndpointHandler {
    public:
        virtual ~EndpointHandler() = default;

        virtual std::optional<std::function<void(data::HttpRequest &, data::HttpResponse &)>> match(request::type request_type, const UserData &data) = 0;

        virtual void addHandler(request::type request_type, const std::regex &regular_expression, std::function<void(data::HttpRequest &, data::HttpResponse &)> function) = 0;

        virtual void addModule(const std::shared_ptr<EndpointHandler> &module_handler) = 0;

        virtual std::vector<std::tuple<request::type, std::regex, std::function<void
            (data::HttpRequest&, data::HttpResponse&)>>> getHandlers() = 0;

    private:
        virtual bool equalsFunction(request::type request_type, const UserData &data, const std::regex &e) = 0;
    };
}; // unit::server::regex
#endif //HANDLER_H
