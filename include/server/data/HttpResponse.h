//
// Created by Kirill Zhukov on 15.11.2023.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "boost/json.hpp"
#include "boost/log/trivial.hpp"
#include "nghttp2/nghttp2.h"

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
#define MAKE_NV(NAME, VALUE)                                                   \
{                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
    NGHTTP2_NV_FLAG_NONE                                                   \
}

    namespace data {
        enum DATA_TYPE {
            NONE = 0,
            BUFFER = 1,
            FD = 2
        };

        class UNITSERVER_API HttpResponse {
            friend class HttpSession;

        public:
            explicit HttpResponse(int32_t stream_id);

            bool writeJSON(const boost::json::value &res);

            bool writeFile(const std::string &file_path);

            bool writeRawData(const uint8_t *buf, size_t length);

            [[nodiscard]] std::optional<std::shared_ptr<std::vector<uint8_t>>> getBuffer() const;

            void getReadOffset(size_t read_offset);

            int getFD() const;

            void addHeader(char *name, char *value);

            void addHeaders(const std::vector<std::pair<std::string, std::string>> &headers);

            std::vector<nghttp2_nv> &getHeaders();

        private:
            static nghttp2_nv make_nv(const char *name, const char *value);

        public:
            DATA_TYPE type = NONE;
            size_t read_offset = 0;

            const int32_t stream_id;
        private:
            std::variant<std::shared_ptr<std::vector<uint8_t>>, int> data;
            std::vector<nghttp2_nv> headers;
        };
    }; // data
}; // unit::server

#endif //HTTPRESPONSE_H
