//
// Created by Kirill Zhukov on 31.10.2023.
//

#include "SessionManager.h"
#include "HttpSession.h"
#include "BasicEndpointHandler.h"
#include "requestTypesStr.h"

namespace unit::server::manager {
    void SessionManager::start_session(boost::asio::ip::tcp::socket socket) {
        const auto session = std::make_shared<
            HttpSession>(std::move(socket), this->key_file_path, this->cert_file_path, this->endpointHandler,
                         this->req_str);
        sessions_.insert(session);
        session->setRemovalCallback([this](std::shared_ptr<HttpSession> s) {
            this->remove_session(s);
        });
        session->start();
    }

    void SessionManager::remove_session(const std::shared_ptr<HttpSession>&session) {
        sessions_.erase(session);
    }

    SessionManager::SessionManager(std::string key_file, std::string cert_file,
                                   const std::shared_ptr<regex::basic::BasicEndpointHandler>&
                                   endpointHandler) : key_file_path(std::move(key_file)),
                                                      cert_file_path(std::move(cert_file)),
                                                      endpointHandler(endpointHandler) {
        this->req_str = std::make_shared<request::RequestTypeStr>();
    }

    void SessionManager::eraseSession(std::shared_ptr<HttpSession>&session) {
        sessions_.erase(session);
    }
}; // unit::server::manager
