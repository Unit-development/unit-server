//
// Created by Kirill Zhukov on 31.10.2023.
//

#ifndef TCPSOCKETTEST_HTTPSESSION_H
#define TCPSOCKETTEST_HTTPSESSION_H

#include <iostream>
#include <map>
#include <array>
#include "boost/asio.hpp"
#include "openssl/ssl.h"
#include "boost/asio/ssl.hpp"
#include "boost/log/trivial.hpp"
#include "nghttp2/nghttp2.h"

#if 0
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/logger.hpp>
#endif

#ifdef __linux__
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#endif

#define OUTPUT_WOULDBLOCK_THRESHOLD (1 << 16)

#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

namespace unit::server {
    namespace callbacks {
        static int on_header_callback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name,
                                      size_t namelen, const uint8_t* value, size_t valuelen, uint8_t flags,
                                      void* user_data);

        static int on_frame_recv_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);

        static int on_stream_close_callback(nghttp2_session* session, int32_t stream_id,
                                            uint32_t error_code, void* user_data);

        static ssize_t send_callback(nghttp2_session* session, const uint8_t* data,
                                     size_t length, int flags, void* user_data);

        static int on_data_chunk_recv_callback(nghttp2_session* session, uint8_t flags,
                                               int32_t stream_id, const uint8_t* data,
                                               size_t len, void* user_data);

        ssize_t raw_data_provider_callback(nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length,
                                       uint32_t* data_flags, nghttp2_data_source* source, void* user_data);

        ssize_t fd_provider_callback(nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length,
                                       uint32_t* data_flags, nghttp2_data_source* source, void* user_data);

        static ssize_t
        send_callback(nghttp2_session* session, const uint8_t* data, size_t length, int flags, void* user_data);

        void write_handler(const boost::system::error_code&error, std::size_t bytes_transferred);

        static int on_begin_headers_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);
    }; // callbacks
    namespace settings {
        static int send_server_connection_header(nghttp2_session* session);
    }; // settings

    namespace regex::basic {
        class BasicEndpointHandler;
    }; // regex::basic

    namespace request {
        class RequestTypeStr;
    }; // request

    namespace data {
        class HttpRequest;
        class HttpResponse;
    }; // data

    class SessionManager;


    // boost::asio::deadline_timer deadline_;
    class HttpSession : public std::enable_shared_from_this<HttpSession> {
    public:
        explicit HttpSession(boost::asio::ip::tcp::socket socket, const std::string&key_file,
                             const std::string&cert_file,
                             const std::shared_ptr<regex::basic::BasicEndpointHandler>&endpoint_handler,
                             const std::shared_ptr<request::RequestTypeStr>&req);

        virtual ~HttpSession();

        void start();

        void push_stream_data(int32_t stream_id, unsigned char* data);

        void eraseData(int32_t stream);

        void setHeaders(int32_t stream_id, const std::string&header, const std::string&header_value);

        void pushEmptyUserData(int32_t stream_id);

        unit::server::data::HttpRequest getUserDataByStream(int32_t stream_id);

        std::map<int32_t,data::HttpRequest>::const_iterator begin() const;

        std::map<int32_t, data::HttpRequest>::const_iterator end() const;

        void setRemovalCallback(std::function<void(std::shared_ptr<HttpSession>)> callback);

        void setCustomErrorPage(std::string &path);

        std::shared_ptr<data::HttpResponse> getErrorPage(int32_t stream_id);

        void selfErase();

    private:
        void do_handshake();

        void do_read();

    public:
        boost::asio::ip::tcp::socket socket_;
        std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ssl_socket;
        std::map<int32_t, data::HttpRequest> streams;
        const std::shared_ptr<regex::basic::BasicEndpointHandler> endpoint_handler;
        std::shared_ptr<request::RequestTypeStr> req_str;

    private:
        nghttp2_session* session{};
        std::string key_file_path;
        std::string cert_file_path;
        std::string error_page_path;
        std::function<void(std::shared_ptr<HttpSession>)> removalCallback;
    };

    namespace utils {
        void send_response(nghttp2_session* session, int32_t stream_id, data::HttpResponse* data, HttpSession *ssl_session);

        static int submit_data(nghttp2_session* session, int32_t stream_id, nghttp2_nv* headers);

        static int submit_fd(nghttp2_session* session, int32_t stream_id, nghttp2_nv* headers);

#if 0
        inline void init_logging() {
            // Logger that writes to a normal file
            logging::add_file_log(
                keywords::file_name = "normal_logs.log",
                keywords::filter = expr::attr<std::string>("LoggerName") != "NullLogger"
            );

            // Logger that writes to /dev/null or NUL
#ifdef _WIN32
            // Windows
            logging::add_file_log(
                keywords::file_name = "NUL",
                keywords::filter = expr::attr<std::string>("LoggerName") == "NullLogger"
            );
#else
            // Linux and Unix-like systems
            logging::add_file_log(
                keywords::file_name = "/dev/null",
                keywords::filter = expr::attr<std::string>("LoggerName") == "NullLogger"
            );
#endif
        }
#endif
    }
} // unit::server


#endif //TCPSOCKETTEST_HTTPSESSION_H
