//
// Created by Kirill Zhukov on 31.10.2023.
//

#include "HttpSession.h"
#include "CTX_util.h"
#include "HttpSessionException.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "types.h"
#include "pathUtil.h"
#include "BasicEndpointHandler.h"
#include "requestTypesStr.h"

int
unit::server::callbacks::on_header_callback(nghttp2_session* session, const nghttp2_frame* frame, const uint8_t* name,
                                            size_t namelen, const uint8_t* value, size_t valuelen, uint8_t flags,
                                            void* user_data) {
#if DEBUG_HEADERS
    std::cout << "Header: " << std::string((char *)name, namelen) << ": " << std::string((char *)value, valuelen)
            << std::endl;
#endif
    const auto ssl_session = static_cast<HttpSession *>(user_data);
    const int32_t stream_id = frame->hd.stream_id;
    try {
        const auto header = std::string((char *)name, namelen);
        const auto header_value = std::string((char *)value, valuelen);
        ssl_session->setHeaders(stream_id, header, header_value);
        if (stream_id != 0 && header[0] == ':' && header[1] == 'p') [[unlikely]]
        {
            ssl_session->streams.at(stream_id).parseUrl(header_value);
        }
    }
    catch (StreamNotFound&e) {
#if DEBUG_HEADERS
        BOOST_LOG_TRIVIAL(error) << e.what();
#endif
    }

    return 0;
}

int
unit::server::callbacks::on_frame_recv_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
#if DEBUG_ON_FRAME
    std::cout << "Received frame: " << frame->hd.type << std::endl;
#endif
    int32_t stream_id = frame->hd.stream_id;
    auto* ssl_session = static_cast<HttpSession *>(user_data);
    switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
                try {
#if DEBUG_HEADERS
                    std::cout << "Headers: " << '\n';
                    for (auto&[key, value]: *ssl_session) {
                        for (auto&[k, v]: value.headers) {
                            std::cout << k << " : " << v << '\n';
                        }
                    }
#endif
#if DEBUG_FRAME_CALLBACK
                    for (auto &[key, value]: *ssl_session) {
                        for (auto &[k, v] : value.headers) {
                            std::cout << "header: " << k << ", header value: " << v << '\n';
                        }
                    }
#endif
                    auto request = ssl_session->streams.at(stream_id);
                    auto match = ssl_session->endpoint_handler->match(
                        ssl_session->req_str->getType(request.headers.at(":method")),
                        request.headers.at(":path"));
                    if (match.has_value()) {
                        auto httpRes = std::make_shared<data::HttpResponse>(stream_id);
                        match.value()(request, *httpRes);
                        utils::send_response(session, stream_id, httpRes.get(), ssl_session);
                    }
                    else {
                        utils::send_response(session, stream_id, ssl_session->getErrorPage(stream_id).get(),
                                             ssl_session);
                    }
                }
                catch (std::exception&e) {
                    BOOST_LOG_TRIVIAL(error) << e.what();
                }
            }
        default:
            break;
    }
    return 0;
}

int unit::server::callbacks::on_data_chunk_recv_callback(nghttp2_session* session, uint8_t flags,
                                                         const int32_t stream_id, const uint8_t* data,
                                                         size_t len, void* user_data) {
    auto* ssl_session = static_cast<HttpSession *>(user_data);
#if DEBUG_CHUNK_RECEIVED
    BOOST_LOG_TRIVIAL(info) << "`on_data_chunk_recv_callback` stream_id: " << stream_id;
#endif
    ssl_session->push_stream_data(stream_id, (unsigned char *)data);
#if DEBUG_CHUNK_RECEIVED
    std::cout << "Received data chunk: " << std::string((char *) data, len) << std::endl;
#endif
    return 0;
}

namespace unit::server {
    HttpSession::HttpSession(boost::asio::ip::tcp::socket socket, const std::string&key_file,
                             const std::string&cert_file,
                             const std::shared_ptr<regex::basic::BasicEndpointHandler>&endpoint_handler,
                             const std::shared_ptr<request::RequestTypeStr>&req)
        : socket_(std::move(socket)), key_file_path(key_file), cert_file_path(cert_file),
          endpoint_handler(endpoint_handler), req_str(req) {
        nghttp2_session_callbacks* callbacks;
        nghttp2_session_callbacks_new(&callbacks);
        nghttp2_session_callbacks_set_send_callback(callbacks, callbacks::send_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(
            callbacks, callbacks::on_begin_headers_callback);
        nghttp2_session_callbacks_set_on_header_callback(callbacks, callbacks::on_header_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(
            callbacks, callbacks::on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
            callbacks, callbacks::on_data_chunk_recv_callback);
        nghttp2_session_server_new(&(this->session), callbacks, this);
        nghttp2_session_callbacks_del(callbacks);
#if 0
        utils::init_logging();
        src::severity_logger_mt<logging::trivial::severity_level> logger;
        src::logger nullLogger;

        // Add custom attributes
        logging::core::get()->add_global_attribute("LoggerName", boost::log::attributes::constant<std::string>(""));

        // Log with normal logger
        logger.add_attribute("LoggerName", boost::log::attributes::constant<std::string>("NormalLogger"));
        BOOST_LOG_SEV(logger, logging::trivial::info) << "This log goes to normal_logs.log";

        // Log with null logger
        nullLogger.add_attribute("LoggerName", boost::log::attributes::constant<std::string>("NullLogger"));
        BOOST_LOG(nullLogger) << "This log goes to /dev/null or NUL";
#endif

    }

    HttpSession::~HttpSession() {
        nghttp2_session_del(this->session);
    }

    void HttpSession::start() {
        do_handshake();
    }

    void HttpSession::do_handshake() {
        auto self = shared_from_this();
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv13_server);
        create_ssl_ctx(ssl_context, this->key_file_path.c_str(), this->cert_file_path.c_str());
#if 0
        auto ssl_socket = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(std::move(this->socket_), ssl_context);
#endif
        this->ssl_socket = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(
            std::move(this->socket_), ssl_context);
        ssl_socket->async_handshake(boost::asio::ssl::stream_base::server,
                                    [this, self](const boost::system::error_code&error) {
                                        if (error) {
#if DEBUG_SSL_HANDSHAKE
                                            BOOST_LOG_TRIVIAL(error) << "SSL Handshake failed: " << error.message();
#endif
                                            return;
                                        }
#if DEBUG_SSL_HANDSHAKE
                                        BOOST_LOG_TRIVIAL(info) << "SSL Handshake successful.";
#endif
                                        // Print SSL details
                                        SSL* ssl = this->ssl_socket->native_handle();
#if DEBUG_SSL_HANDSHAKE
                                        BOOST_LOG_TRIVIAL(info) << "SSL Version: " << SSL_get_version(ssl);
                                        BOOST_LOG_TRIVIAL(info) << "SSL Cipher: " << SSL_get_cipher_name(ssl);
#endif
                                        // Get ALPN protocol to check for HTTP/2
                                        const unsigned char* alpn_protocol = nullptr;
                                        unsigned int alpn_len = 0;
                                        SSL_get0_alpn_selected(ssl, &alpn_protocol, &alpn_len);
                                        if (alpn_len == 2 && memcmp("h2", alpn_protocol, 2) == 0) {
#if DEBUG
                                            BOOST_LOG_TRIVIAL(info) << "ALPN selected protocol: HTTP/2";
#endif
                                        }
                                        else {
#if DEBUG
                                            BOOST_LOG_TRIVIAL(info) << "ALPN did not select HTTP/2. Selected: "
                                                                    << std::string(
                                                                       (const char *)alpn_protocol, alpn_len);
#endif
                                        }
                                        settings::send_server_connection_header(this->session);
                                        do_read();
                                    });
    }

    void HttpSession::do_read() {
        std::array<unsigned char, 8192> data{};
        boost::system::error_code ec;
        this->ssl_socket->async_read_some(boost::asio::buffer(data),
                                          [this, &data](boost::system::error_code ec, std::size_t length) {
                                              if (!ec) {
#if DEBUG_ON_RECEIVE
                                                  BOOST_LOG_TRIVIAL(info) << "Data received: " << length << " bytes.";
#endif
                                                  ssize_t rv = nghttp2_session_mem_recv(session, data.data(), length);
                                                  if (rv < 0) {
#if DEBUG_ON_RECEIVE
                                                      BOOST_LOG_TRIVIAL(error) << "nghttp2_session_mem_recv failed: " <<
 nghttp2_strerror(rv);
#endif
                                                      return;
                                                  }
                                                  do_read();
                                              }
#ifdef DEBUG
                                              else {
                                                  BOOST_LOG_TRIVIAL(error) << "`async_read_some`: " << ec.what();
                                                  this->selfErase();
                                              }
#endif
                                          });
    }

    void HttpSession::push_stream_data(int32_t stream_id, unsigned char* data) {
        if (!this->streams.contains(stream_id)) {
            this->streams.emplace_hint(this->streams.begin(), stream_id,
                                       data::HttpRequest{
                                           false,
                                           std::vector<unsigned char>(data, data + strlen(
                                                                                reinterpret_cast<const char *>(
                                                                                    data))),
                                           stream_id
                                       });
        }
        else {
            this->streams.at(stream_id).data.push_back(*data);
        }
    }

    std::map<int32_t, data::HttpRequest>::const_iterator HttpSession::begin() const {
        return this->streams.begin();
    }

    std::map<int32_t, data::HttpRequest>::const_iterator HttpSession::end() const {
        return this->streams.end();
    }

    void HttpSession::setRemovalCallback(std::function<void(std::shared_ptr<HttpSession>)> callback) {
        this->removalCallback = callback;
    }

    void HttpSession::setCustomErrorPage(std::string&path) {
        this->error_page_path = path;
    }

    std::shared_ptr<data::HttpResponse> HttpSession::getErrorPage(int32_t stream_id) {
        auto httpRes = std::make_shared<data::HttpResponse>(stream_id);
        if (this->error_page_path.empty()) {
            const std::string data = "<html><head><title>404</title></head>"
                    "<body><h1>404 Not Found</h1></body></html>";
            httpRes->writeRawData(reinterpret_cast<const uint8_t *>(data.data()), data.length());
            httpRes->addHeader((char *)":status", (char *)"404");
            httpRes->addHeader((char *)"Content-Type", (char *)"text/html; charset=utf-8");
        }
        else {
            httpRes->writeFile(this->error_page_path);
            httpRes->addHeader((char *)":status", (char *)"404");
            httpRes->addHeader((char *)"Content-Type", (char *)"text/html; charset=utf-8");
        }
        return httpRes;
    }

    void HttpSession::selfErase() {
        this->removalCallback(shared_from_this());
    }

    data::HttpRequest HttpSession::getUserDataByStream(int32_t stream_id) {
        try {
            return this->streams.at(stream_id);
        }
        catch (std::out_of_range&e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            throw StreamNotFound("There is no stream with provided stream_id");
        }
    }

    void HttpSession::eraseData(int32_t stream) {
        this->streams.erase(stream);
    }

    void HttpSession::setHeaders(int32_t stream_id, const std::string&header, const std::string&header_value) {
        try {
            this->pushEmptyUserData(stream_id);
            this->streams.at(stream_id).headers[header] = header_value;
        }
        catch (std::out_of_range&e) {
            BOOST_LOG_TRIVIAL(error) << e.what();
            throw StreamNotFound("There is no stream with provided stream_id");
        }
    }

    void HttpSession::pushEmptyUserData(const int32_t stream_id) {
        this->streams.try_emplace(this->streams.begin(), stream_id, unit::server::data::HttpRequest(stream_id));
    }
}; // unit::server

ssize_t unit::server::callbacks::raw_data_provider_callback(nghttp2_session* session, int32_t stream_id,
                                                            uint8_t* buf,
                                                            size_t length,
                                                            uint32_t* data_flags, nghttp2_data_source* source,
                                                            void* user_data) {
    (void)session;
    (void)stream_id;
    auto* data = static_cast<data::HttpResponse *>(source->ptr);
    auto* ssl_session = static_cast<HttpSession *>(user_data);

    if (data->type == data::BUFFER) {
        auto buffer = data->getBuffer();
        if (buffer.has_value()) {
            // Проверяем, есть ли еще данные для отправки
            if (data->read_offset >= buffer->get()->size()) {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;
                return 0; // Все данные отправлены
            }

            // Вычисляем, сколько данных можно скопировать в этом вызове
            size_t data_left = buffer->get()->size() - data->read_offset;
            size_t copy_len = (data_left < length) ? data_left : length;

            // Копируем данные в буфер
            memcpy(buf, buffer->get()->data() + data->read_offset, copy_len);
            data->read_offset += copy_len; // Обновляем смещение

            // Проверяем, достигли ли мы конца данных
            if (data->read_offset >= buffer->get()->size()) {
                *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            }
            return static_cast<ssize_t>(copy_len); // Возвращаем количество скопированных байт
        }
    }
    else {
        const int fd = data->getFD();
        if (fd < 0) {
            BOOST_LOG_TRIVIAL(error) << "file descriptor corrupted";
        }
        ssize_t r;

        while ((r = read(fd, buf, length)) == -1 && errno == EINTR) {
        }

        if (r == -1) {
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }

        if (r == 0) {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        }

        return r;
    }
}

void unit::server::utils::send_response(nghttp2_session* session, const int32_t stream_id, data::HttpResponse* data,
                                        HttpSession* ssl_session) {
    nghttp2_data_provider provider;
    provider.source.ptr = data;
    provider.read_callback = callbacks::raw_data_provider_callback;

    const std::vector<nghttp2_nv> headers = data->getHeaders();
    int rv = nghttp2_submit_response(session, stream_id, headers.data(), headers.size(), &provider);

    if (rv == 0) {
        std::vector<uint8_t> dt;
        while (true) {
            const uint8_t *dst;
            const size_t length = nghttp2_session_mem_send(session, &dst);
            if (length == 0) {
                BOOST_LOG_TRIVIAL(info) << "Data written to socket";
                break;
            }
            dt.insert(dt.end(), dst, dst + length);
        }
        boost::system::error_code ec;
        boost::asio::async_write(*ssl_session->ssl_socket, boost::asio::buffer(dt.data(), dt.size()), callbacks::write_handler);
    }
    else {
        BOOST_LOG_TRIVIAL(error) << "send_response `nghttp2_submit_response`: " << nghttp2_strerror(rv);
        ssl_session->selfErase();
    }
}

static ssize_t unit::server::callbacks::send_callback(nghttp2_session* session, const uint8_t* data,
                                                      size_t length, int flags, void* user_data) {
    boost::system::error_code ec;
    auto* ssl_session = static_cast<HttpSession *>(user_data);
#if DEBUG_SOCKET_WRITING
    BOOST_LOG_TRIVIAL(info) << "Sending data of length: " << length;
#endif
    // boost::asio::write(*ssl_session->ssl_socket, boost::asio::buffer(data, length), ec);
    // if (ec) {
    //     BOOST_LOG_TRIVIAL(error) << "`send_callback: `" << ec.what();
    // }
    boost::asio::async_write(*ssl_session->ssl_socket, boost::asio::buffer(data, length),
                             [ssl_session, length, data](const boost::system::error_code&ec, std::size_t n) {
                                 if (ec) {
                                     BOOST_LOG_TRIVIAL(error) << "`send_callback`: " << ec.what();
                                     for (int i = 0; i < length; i++) {
                                         std::cout << (char) data[i];
                                     }
                                     std::cout << '\n';
                                     // ssl_session->selfErase();
                                 }
                             });

    return static_cast<ssize_t>(length);
}

void unit::server::callbacks::write_handler(const boost::system::error_code&error, std::size_t bytes_transferred) {
    if (!error) {
#if DEBUG_SOCKET_WRITING
        BOOST_LOG_TRIVIAL(info) << "Successfully wrote " << bytes_transferred << " bytes.";
#endif
    }
    else {
#if DEBUG_SOCKET_WRITING
        BOOST_LOG_TRIVIAL(error) << "Write failed: " << error.message();
#endif
    }
}

int unit::server::callbacks::on_stream_close_callback(nghttp2_session* session, const int32_t stream_id,
                                                      uint32_t error_code, void* user_data) {
    auto* ssl_session = static_cast<HttpSession *>(user_data);
    BOOST_LOG_TRIVIAL(info) << "Stream closed: " << stream_id << ", error code: " << error_code;
    if (!ssl_session) {
        return 0;
    }
    ssl_session->streams.erase(stream_id);
    if (stream_id == 1) {
        ssl_session->selfErase();
    }
    return 0;
}

int unit::server::settings::send_server_connection_header(nghttp2_session* session) {
    nghttp2_settings_entry iv[1] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
    };
    int rv = nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, iv, std::size(iv));
    if (rv != 0) {
        BOOST_LOG_TRIVIAL(error) << "`settings`: " << nghttp2_strerror(rv);
        return -1;
    }
    rv = nghttp2_session_send(session);
    if (rv != 0) {
        BOOST_LOG_TRIVIAL(error) << "`settings`: " << nghttp2_strerror(rv);
        return -1;
    }
    return 0;
}

int unit::server::callbacks::on_begin_headers_callback(nghttp2_session* session, const nghttp2_frame* frame,
                                                       void* user_data) {
    auto ssl_session = static_cast<HttpSession *>(user_data);
    if (frame->hd.type != NGHTTP2_HEADERS ||
        frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        return 0;
    }
#if DEBUG_STREAM_PRINTING
    BOOST_LOG_TRIVIAL(info) << "stream_id in `on_begin_headers_callback`: " << frame->hd.stream_id;
#endif
    nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, user_data);
    return 0;
}
