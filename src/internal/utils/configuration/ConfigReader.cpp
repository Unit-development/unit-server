//
// Created by Kirill Zhukov on 15.11.2023.
//

#include "ConfigReader.h"

unit::server::configuration::ConfigReader::ConfigReader(const std::string&config_path) {
    try {
        this->res = toml::parse_file(config_path);
    }
    catch (toml::v3::ex::parse_error&e) {
        BOOST_LOG_TRIVIAL(error) << "`ConfigReader` constructor error: " << e.what();
        exit(9);
    }
}

std::string unit::server::configuration::ConfigReader::getKeyFilePath() {
    if (!this->res.contains("certs") || !this->res.get_as<toml::table>("certs")->contains("key_file")) {
        BOOST_LOG_TRIVIAL(error) << "No key file was provided";
        exit(9);
    }

    auto t = this->res["test"];
    return this->res["certs"]["key_file"].as_string()->get();
}

std::string unit::server::configuration::ConfigReader::getPemFilePath() {
    if (!this->res.contains("certs") || !this->res.get_as<toml::table>("certs")->contains("cert_file")) {
        BOOST_LOG_TRIVIAL(error) << "No cert file was provided";
        exit(9);
    }
    return this->res["certs"]["cert_file"].as_string()->get();
}

int64_t unit::server::configuration::ConfigReader::getPort() {
    if (!this->res.contains("net") || !this->res.get_as<toml::table>("net")->contains("port")) {
        BOOST_LOG_TRIVIAL(error) << "No port was provided";
        exit(9);
    }
    return this->res["net"]["port"].as_integer()->get();
}

toml::node_view<toml::node> unit::server::configuration::ConfigReader::getKey(const std::string&key) {
    return this->res[key];
}

int64_t unit::server::configuration::ConfigReader::getThreads() {
    if (!this->res.contains("server") || !this->res.get_as<toml::table>("server")->contains("threads")) {
        BOOST_LOG_TRIVIAL(error) << "No cert file was provided";
        exit(9);
    }
    return this->res["server"]["threads"].as_integer()->get();
}
