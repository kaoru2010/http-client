#pragma once

#include <memory>
#include <boost/asio.hpp>

#include <network/protocols/http/response_header.hpp>
#include <network/protocols/http/response_status.hpp>

#ifndef MAX_CHUNK_SIZE_BYTES
#  define MAX_CHUNK_SIZE_BYTES 32
#endif

namespace network { namespace protocols { namespace http {

boost::system::error_code parse_response_header(const char *buf, size_t size, http_response_context& ctx);

template <
    typename AsyncReadSream,
    typename Handler
>
void async_read_header(AsyncReadSream& socket, boost::asio::streambuf& buf, http_response_context& ctx, Handler&& handler)
{
    boost::asio::async_read_until(socket, buf, "\r\n\r\n", [&, handler=std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec) {
            return handler(ec, bytes_transferred);
        }

        boost::system::error_code error = parse_response_header(
            boost::asio::buffer_cast<const char *>(buf.data()),
            bytes_transferred,
            ctx);

        if ( !error) {
            buf.consume(bytes_transferred);
        }

        return handler(error, bytes_transferred);
    });
}

void async_read_body();

template <
    typename AsyncReadSream,
    typename Handler
>
void async_read_chunk_size(AsyncReadSream& socket, boost::asio::streambuf& buf, size_t& chunk_size, Handler&& handler)
{
    boost::asio::async_read_until(socket, buf, "\r\n", [&, handler=std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec) {
            return handler(ec);
        }

        if (bytes_transferred - 2 > MAX_CHUNK_SIZE_BYTES - 1) {
            return handler(boost::system::make_error_code(HTTP_RESPONSE_MALFORMED));
        }

        char tmp_buf[MAX_CHUNK_SIZE_BYTES];
        memcpy(tmp_buf, boost::asio::buffer_cast<const char *>(buf.data()), bytes_transferred - 2);
        tmp_buf[bytes_transferred - 2] = 0;
        char *endptr = 0;
        chunk_size = strtol(tmp_buf, &endptr, 16);

        if (! (*endptr == 0 || *endptr == ';')) {
            return handler(boost::system::make_error_code(HTTP_RESPONSE_MALFORMED));
        }

        buf.consume(bytes_transferred);

        return handler(ec);
    });
}

class match_size
{
    using iterator = boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type>;

public:
    explicit match_size(size_t size)
    :   size_(size)
    {}

    auto operator()(iterator begin, iterator end) const -> std::pair<iterator, bool>
    {
        size_t i = 0;
        for (iterator ite = begin; ite != end; i++, ite++) {
            if (i == size_) {
                return std::make_pair(ite, true);
            }
        }

        return std::make_pair(end, false);
    }

private:
    size_t size_;
};

template <
    typename AsyncReadSream,
    typename Handler
>
void async_read_until_with_size(AsyncReadSream& socket, boost::asio::streambuf& buf, size_t size, Handler&& handler)
{
    boost::asio::async_read_until(socket, buf, match_size(size), std::forward<Handler>(handler));
}

template <
    typename AsyncReadSream,
    typename Handler
>
void async_read_chunk_data(AsyncReadSream& socket, boost::asio::streambuf& buf, size_t chunk_size, Handler&& handler)
{
    async_read_until_with_size(socket, buf, chunk_size + 2, [&, handler=std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec) {
            return handler(ec, bytes_transferred);
        }
        else {
            handler(ec, bytes_transferred - 2);
            buf.consume(2);
            return;
        }
    });
}

template <
    typename AsyncReadSream,
    typename Handler
>
void async_read_trailer(AsyncReadSream& socket, boost::asio::streambuf& buf, Handler&& handler)
{
    auto callback = [&, handler=std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec) {
            return handler(ec);
        }

        buf.consume(bytes_transferred);

        if (bytes_transferred == 2) {
            return handler(ec);
        }
        else {
            async_read_trailer(socket, buf, handler);
        }
    };

    boost::asio::async_read_until(socket, buf, "\r\n", std::move(callback));
}

}}} // namespace network { namespace protocols { namespace http {

namespace boost { namespace asio {
    template <> struct is_match_condition<network::protocols::http::match_size> : public boost::true_type {};
}} // namespace boost { namespace asio {
