#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>
#include <boost/optional.hpp>

namespace network { namespace protocols { namespace http {

enum http_protocol_version_t
{
    HTTP_1_0,
    HTTP_1_1,
};

using header_list_type = std::multimap<std::string, std::string>;

struct http_response_context
{
    http_protocol_version_t protocol_version = HTTP_1_0;
    int status_code = 0;
    boost::optional<std::string> content_type;
    boost::optional<size_t> content_length = {};
    bool chunked = false;
    header_list_type headers;
    bool keep_alive = false;

    void accept_header_key(const char *key)
    {
        BOOST_ASSERT(key != NULL);
        tmp_key_ = key;
    }

    void accept_header_value(const char *value) {
        BOOST_ASSERT(value != NULL);
        std::string key;
        key.swap(tmp_key_);
        headers.insert(make_pair(std::move(key), std::string(value)));
    }

    void accept_content_type(const char *content_type) {
        if (content_type) {
            this->content_type = std::string(content_type);
        }
    }

    void accept_transfer_coding(const char *transfer_coding) {
        if (strncmp("chunked", transfer_coding, 7) == 0) {
            if (transfer_coding[7] == ';' || transfer_coding[7] == 0) {
                this->chunked = true;
                return;
            }
        }

        this->chunked = false;
    }

private:
    std::string tmp_key_;
};

}}} // namespace network { namespace protocols { namespace http {
