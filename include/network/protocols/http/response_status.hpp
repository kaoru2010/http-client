#pragma once

#include <boost/system/error_code.hpp>

namespace network { namespace protocols { namespace http {

enum response_status_t {
    HTTP_RESPONSE_OK,
    HTTP_RESPONSE_MALFORMED,
};

}}} // namespace network { namespace protocols { namespace http {

namespace boost { namespace system {

class http_response_status_error_category : public error_category
{
public:
    virtual const char* name() const noexcept {
        return "http_response_status";
    }

    virtual std::string message(int ev) const {
        using namespace network::protocols::http;

        switch (ev) {
            case HTTP_RESPONSE_OK: return "OK";
            case HTTP_RESPONSE_MALFORMED: return "MALFORMED";
            default: BOOST_ASSERT(false); return "UNKNOWN";
        }
    }
};

inline
http_response_status_error_category& get_http_response_status_error_category() {
    static http_response_status_error_category instance;
    return instance;
}

template<>
struct is_error_condition_enum<network::protocols::http::response_status_t>
{
    static const bool value = true;
};

inline
error_condition make_error_condition(network::protocols::http::response_status_t error) noexcept {
    return error_condition(error, get_http_response_status_error_category());
}

inline
error_code make_error_code(network::protocols::http::response_status_t ev) {
    return error_code(ev, get_http_response_status_error_category());
}

}} // namespace boost { namespace system {
