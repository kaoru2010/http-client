#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <algorithm>
#include <iterator>
#include <ostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/optional.hpp>

#include <network/protocols/http/http_client_base.hpp>

namespace network { namespace protocols { namespace http {

class http_request
{
    std::string method_;
    std::string uri_;
    std::string hostname_;

public:
    explicit http_request(std::string const& method, std::string const& uri, std::string const& hostname)
    :   method_(method)
    ,   uri_(uri)
    ,   hostname_(hostname)
    {}

    virtual ~http_request() = 0;
    virtual void write_header(std::ostream& out) const {}
    virtual void write_body(std::ostream& out) const {}

    friend std::ostream& operator<<(std::ostream& out, http_request const& self)
    {
        out << self.method_ << " " << self.uri_ << " HTTP/1.1\r\n";
        out << "Host: " << self.hostname_ << "\r\n";
        self.write_header(out);
        out << "\r\n";
        self.write_body(out);

        return out;
    }

    std::string method() const { return method_; }
    std::string hostname() const { return hostname_; }
    std::string uri() const { return uri_; }
};

inline
http_request::~http_request()
{
}

class http_get_request
    :   public http_request
    ,   std::enable_shared_from_this<http_get_request>
{
public:
    explicit http_get_request(std::string const& method, std::string const& uri, std::string const& hostname)
    :   http_request(method, uri, hostname)
    {}
};

class http_post_request
    :   public http_request
    ,   std::enable_shared_from_this<http_post_request>
{
    boost::optional<std::string> content_type_;
    boost::optional<std::string> post_data_;

public:
    explicit http_post_request(std::string const& method, std::string const& uri, std::string const& hostname)
    :   http_request(method, uri, hostname)
    {}

    virtual void write_header(std::ostream& out) const
    {
        out << "Content-Length: " << (post_data_ ? post_data_->size() : 0) << "\r\n";
        out << "Content-Type: " << (content_type_ ? *content_type_ : "application/x-www-form-urlencoded") << "\r\n";
    }

    virtual void write_body(std::ostream& out) const
    {
        using namespace std;

        if (post_data_) {
            copy(post_data_->begin(), post_data_->end(), ostreambuf_iterator<char>(out));
        }
    }

    void set_content_type(boost::optional<std::string>&& content_type) {
        content_type_.swap(content_type);
    }

    void set_content_type(boost::optional<std::string> const& content_type) {
        content_type_ = content_type;
    }

    void set_post_data(boost::optional<std::string>&& post_data) {
        post_data_.swap(post_data);
    }

    void set_post_data(boost::optional<std::string> const& post_data) {
        post_data_ = post_data;
    }
};

class simple_http_request
{
public:
    simple_http_request()
    :   request_buf_()
    ,   request_ptr_()
    {}

    template <typename AsyncWriteStream, typename Handler>
    bool async_request(AsyncWriteStream& socket, Handler&& handler)
    {
        if ( !request_ptr_) return false;

        prepare_request_buf();
        boost::asio::async_write(socket, request_buf_, std::forward<Handler>(handler));
        return true;
    }

    void set_request(std::shared_ptr<http_request> const& request_ptr)
    {
        request_ptr_ = request_ptr;
    }

    void set_request(std::shared_ptr<http_request>&& request_ptr)
    {
        request_ptr_ = std::move(request_ptr);
    }

    std::shared_ptr<http_request> get_request() const
    {
        return request_ptr_;
    }

private:
    void prepare_request_buf()
    {
        std::ostream request_stream(&request_buf_);
        request_stream << *request_ptr_;
    }

private:
    boost::asio::streambuf request_buf_;
    std::shared_ptr<http_request> request_ptr_;
};

}}} // namespace network { namespace protocols { namespace http {
