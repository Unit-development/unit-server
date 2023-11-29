//
// Created by Kirill Zhukov on 18.11.2023.
//

#ifndef PATHUTIL_H
#define PATHUTIL_H

#include <string>
#include <utility>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>


namespace unit::server {
    inline std::pair<std::string, std::string> getPathAndFileName(const std::string& url_str) {
        std::string path;

        // Check if the URL is a full URL or a relative path
        if (url_str.find("://") != std::string::npos) {
            // Parse as a full URL
            boost::urls::url url = boost::urls::parse_uri(url_str).value();
            auto path_view = url.encoded_path();
            path.assign(path_view.begin(), path_view.end()); // Convert pct_string_view to std::string
        } else {
            // Treat as a relative path
            path = url_str;
        }

        size_t lastSlashPos = path.rfind('/');
        size_t dotPos = path.rfind('.');

        if (dotPos != std::string::npos && (lastSlashPos == std::string::npos || dotPos > lastSlashPos)) {
            std::string fileName = path.substr(lastSlashPos + 1); // Extract file name

            // Path excluding file name, retaining '/' if the file is at the root
            std::string pathExcludingFileName = lastSlashPos == 0 ? "/" : path.substr(0, lastSlashPos);

            return {pathExcludingFileName, fileName};
        }

        return {path, ""}; // No file name, return full path
    }
}; // unit::server

#endif //PATHUTIL_H
