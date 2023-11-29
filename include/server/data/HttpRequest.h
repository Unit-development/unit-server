//
// Created by Kirill Zhukov on 10.11.2023.
//

#ifndef TCPSOCKETTEST_HTTPREQUEST_H
#define TCPSOCKETTEST_HTTPREQUEST_H
#include <iostream>
#include <boost/url.hpp>

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
        class UNITSERVER_API HttpRequest {
        public:
            HttpRequest(bool isFinished, const std::vector<unsigned char> &data, int32_t stream_id);

            explicit HttpRequest(int32_t stream_id);

            [[nodiscard]] int32_t getStreamId() const;

            [[nodiscard]] const std::vector<unsigned char> &getData();

            void parseUrl(const std::string &url);

            int32_t getStreamId();

        public:
            std::unordered_map<std::string, std::string> headers{};
            std::vector<unsigned char> data;
            boost::url_view url;
        private:
            const int32_t stream_id;
        };
    }; // data
}; // unit::server

#endif //TCPSOCKETTEST_HTTPREQUEST_H
