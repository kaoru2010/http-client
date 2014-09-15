#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/optional.hpp>

#include <network/protocols/http/http_client_base.hpp>
#include <network/protocols/http/response_header.hpp>
#include <network/protocols/http/response_parser.hpp>

namespace network { namespace protocols { namespace http {

#include <boost/asio/yield.hpp>
template <
    typename SocketPolicy,
    typename RequestPolicy,
    typename ResponsePolicy
>
class http_client
    :   public std::enable_shared_from_this<http_client<SocketPolicy, RequestPolicy, ResponsePolicy>>
    ,   private boost::asio::coroutine
    ,   public SocketPolicy
    ,   public RequestPolicy
    ,   public ResponsePolicy
    ,   public http_client_base
{
    boost::asio::io_service& io_service_;
    boost::asio::streambuf response_buf_;
    http_response_context response_ctx_;
    size_t chunk_size_;

public:
    explicit http_client(boost::asio::io_service& io_service)
    :   SocketPolicy(io_service)
    ,   io_service_(io_service)
    ,   chunk_size_()
    {}

    void start()
    {
        (*this)({}, 0);
    }

    void operator()(boost::system::error_code const& ec, std::size_t bytes_transferred)
    {
        auto call_self = [shared_this = this->shared_from_this()](boost::system::error_code const& ec = {}, std::size_t bytes_transferred = 0)
        {
            (*shared_this)(ec, bytes_transferred);
        };

        if (ec)
            goto yield_break;

        reenter(this) {
            yield this->async_connect(this->socket(), call_self);

            do {
                yield this->async_request(this->socket(), call_self);

                yield async_read_header(this->socket(), response_buf_, response_ctx_, call_self);

                if (response_ctx_.chunked) {
                    while (1) {
                        yield async_read_chunk_size(this->socket(), response_buf_, chunk_size_, call_self);
                        if (chunk_size_ == 0) {
                            yield async_read_trailer(this->socket(), response_buf_, call_self);
                            goto yield_break;
                        }

                        yield async_read_chunk_data(this->socket(), response_buf_, chunk_size_, call_self);
                        // TODO
                        response_buf_.consume(bytes_transferred);
                    }
                }
                else if (response_ctx_.content_length) {
                    yield async_read_until_with_size(this->socket(), response_buf_, *response_ctx_.content_length, call_self);
                    // TODO
                    response_buf_.consume(bytes_transferred);
                    goto yield_break;
                }
                else {
                    yield async_read(this->socket(), response_buf_, call_self);
                    // TODO
                    // Ignore EOF
                    response_buf_.consume(bytes_transferred);
                    goto yield_break;
                }

            } while (response_ctx_.keep_alive);
        }

        return;

yield_break:
        return;
    }
};
#include <boost/asio/unyield.hpp>

}}} // namespace network { namespace protocols { namespace http {
