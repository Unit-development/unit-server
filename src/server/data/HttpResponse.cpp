//
// Created by Kirill Zhukov on 15.11.2023.
//

#include "HttpResponse.h"

unit::server::data::HttpResponse::HttpResponse(const int32_t stream_id) : stream_id(stream_id), type(NONE) {}

bool unit::server::data::HttpResponse::writeJSON(const boost::json::value&res) {
    if (this->type != NONE) {
        return false;
    }
    boost::json::serializer sr;
    sr.reset(&res);

    std::vector<uint8_t> temp_buffer;
    while (!sr.done()) {
        char buf[4096];
        auto sv = sr.read(buf, sizeof(buf));
        temp_buffer.insert(temp_buffer.end(), sv.data(), sv.data() + sv.size());
    }

    this->data = std::make_shared<std::vector<uint8_t>>(std::move(temp_buffer));
    this->type = BUFFER;
    return true;
}

bool unit::server::data::HttpResponse::writeFile(const std::string&file_path) {
    if (this->type != NONE) {
        return false;
    }
    int fd = open(file_path.c_str(), O_RDONLY);

    if (fd == -1) {
        const std::string data = "<html><head><title>404</title></head>"
                             "<body><h1>404 Not Found</h1></body></html>";
        BOOST_LOG_TRIVIAL(error) << "File not opened: " << file_path << ", error: " << strerror(errno);
        this->writeRawData(reinterpret_cast<const uint8_t*>(data.data()), data.length());
        this->addHeader((char *) ":status", (char *) "404");
        this->addHeader((char *) "Content-Type", (char *) "text/html; charset=utf-8");
        return false;
    }
    this->data = fd;
    this->type = FD;
    return true;
}

bool unit::server::data::HttpResponse::writeRawData(const uint8_t* buf, const size_t length) {
    if (this->type != NONE) {
        return false;
    }
    this->data = std::make_shared<std::vector<uint8_t>>(buf, buf + length);
    this->type = BUFFER;
    return true;
}

std::optional<std::shared_ptr<std::vector<uint8_t>>> unit::server::data::HttpResponse::getBuffer() const {
    if (std::holds_alternative<std::shared_ptr<std::vector<uint8_t>>>(data)) {
        return std::get<std::shared_ptr<std::vector<uint8_t>>>(data);
    } else {
        return std::nullopt;
    }
}

int unit::server::data::HttpResponse::getFD() const {
    if (std::holds_alternative<int>(this->data)) {
        return std::get<int>(this->data);
    } else {
        return -1;
    }
}

void unit::server::data::HttpResponse::addHeader(char *name, char *value) {
    this->headers.push_back({(uint8_t *) name, (uint8_t*) value, std::strlen(name), std::strlen(value), NGHTTP2_NV_FLAG_NONE});
}

void unit::server::data::HttpResponse::addHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
    for (auto &[header, value] : headers) {
        this->headers.push_back(MAKE_NV(header.c_str(), value.c_str()));
    }
}

void unit::server::data::HttpResponse::getReadOffset(const size_t read_offset) {
    this->read_offset = read_offset;
}

nghttp2_nv unit::server::data::HttpResponse::make_nv(const char *name, const char *value) {
    return {
        (uint8_t *)(name),
        (uint8_t *)(value),
        strlen(name),
        strlen(value),
        NGHTTP2_NV_FLAG_NONE
    };
}

std::vector<nghttp2_nv>& unit::server::data::HttpResponse::getHeaders() {
    if (this->headers.empty()) {
        this->headers.push_back(MAKE_NV(":status", "200"));
        this->headers.push_back(MAKE_NV("content-type", "text/plain"));
    }
    return this->headers;
}