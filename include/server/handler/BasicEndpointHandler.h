//
// Created by Kirill Zhukov on 20.11.2023.
//

#ifndef BASICENDPOINTHANDLER_H
#define BASICENDPOINTHANDLER_H

#include <future>
#include <functional>
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

namespace unit::server::regex::basic {
    class UNITSERVER_API BasicEndpointHandler final : public EndpointHandler<> {
    public:
        BasicEndpointHandler() = default;
        ~BasicEndpointHandler() override = default;

        std::future<std::optional<std::function<void(data::HttpRequest&, data::HttpResponse&)>>> matchAsync(
            const request::type&reqType, const std::string&data);

        std::optional<std::function<void (data::HttpRequest&, data::HttpResponse&)>> match(
            request::type request_type, const std::string&data) override;

        void addHandler(request::type request_type, const std::regex&regular_expression,
                        std::function<void (data::HttpRequest&, data::HttpResponse&)> function) override;

        void addModule(const std::shared_ptr<EndpointHandler> &module_handler) override;

        std::vector<std::tuple<request::type, std::regex, std::function<void
            (data::HttpRequest&, data::HttpResponse&)>>> getHandlers() override;

    private:
        bool equalsFunction(request::type request_type, const std::string&data, const std::regex&e) override;

    protected:
        std::vector<std::tuple<request::type, std::regex, std::function<void
            (data::HttpRequest&, data::HttpResponse&)>>> regexes;
    };
}; // unit::server::regex::basic

#endif //BASICENDPOINTHANDLER_H
