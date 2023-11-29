//
// Created by Kirill Zhukov on 15.11.2023.
//

#include "Server.h"
#include <utility>
#include "ConfigReader.h"
#include "HttpRequest.h"
#include "BasicEndpointHandler.h"
#include "SessionManager.h"


unit::server::handler::Server::Server(const std::string&config_path) {
    this->config = std::make_shared<configuration::ConfigReader>(config_path);
    this->endpoint_handler = std::make_shared<regex::basic::BasicEndpointHandler>();
}

void unit::server::handler::Server::handle(const request::type request_type, const std::string&endpoint,
                                           const std::function<void (data::HttpRequest&,
                                                                     data::HttpResponse&)>&function) {
    this->endpoint_handler->addHandler(request_type, std::regex(endpoint), function);
}

void unit::server::handler::Server::addModule(module::HandlerModule& module) {
    this->endpoint_handler->addModule(module.getNativeHandler());
}

void unit::server::handler::Server::start() {
    try {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::acceptor acceptor(
            io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), this->config->getPort()));
        manager::SessionManager manager = manager::SessionManager(this->config->getKeyFilePath(),
                                                                  this->config->getPemFilePath(),
                                                                  this->endpoint_handler);
        std::function<void()> do_accept;
        do_accept = [&]() {
            auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
            acceptor.async_accept(*socket, [&, socket](boost::system::error_code ec) {
                if (!ec) {
#if DEBUG_CONNECTION
                    BOOST_LOG_TRIVIAL(info) << "Connection accepted.";
#endif
                    manager.start_session(std::move(*socket));
                }
                else {
#if DEBUG_CONNECTION
                    BOOST_LOG_TRIVIAL(error) << "ERROR: " << ec.what();
#endif
                }
                do_accept();
            });
        };
        do_accept();
        int64_t threads_num = this->config->getThreads();
        this->iocs.reserve(threads_num);
        for (auto i = threads_num - 1; i > 0; --i)
            this->iocs.emplace_back(
                [&io_context] {
                    io_context.run();
                });
        io_context.run();
        for (auto&t: this->iocs) t.join();
        io_context.run();
        exit(9);
    }
    catch (std::exception&e) {
        BOOST_LOG_TRIVIAL(error) << e.what();
        exit(9);
    }
}
