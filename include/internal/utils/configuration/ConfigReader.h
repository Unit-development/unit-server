//
// Created by Kirill Zhukov on 15.11.2023.
//

#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <iostream>
#include "toml.hpp"
#include <boost/log/trivial.hpp>

namespace unit::server {
    namespace configuration {
        namespace error {
            class WrongConfig final : public std::exception {
            public:
                // Constructor (C++11 onward)
                explicit WrongConfig(std::string msg) : message(std::move(msg)) {}

                // Override the what() method from the base class
                [[nodiscard]] const char *what() const noexcept override;

            private:
                std::string message;
            };
        }; // error

        class ConfigReader {
        public:
            explicit ConfigReader(const std::string &config_path);

            std::string getKeyFilePath();

            std::string getPemFilePath();

            int64_t getPort();

            int64_t getThreads();

            toml::node_view<toml::node> getKey(const std::string &key);

        private:
            toml::parse_result res;
        };

    }; // configuration
}; // unit::server


#endif //CONFIGREADER_H
