#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>

namespace network { namespace protocols { namespace http {

class http_socket
{
    boost::asio::ip::tcp::resolver resolver_;
    boost::optional<boost::asio::ip::tcp::resolver::query> query_;
    boost::asio::ip::tcp::socket socket_;

public:
    http_socket(boost::asio::io_service& io_service)
    :   resolver_(io_service)
    ,   query_()
    ,   socket_(io_service)
    {}

    void set_host_port(std::string const host, std::string const& port)
    {
        query_ = boost::asio::ip::tcp::resolver::query(host, port);
    }

    template <typename Handler>
    void async_connect(Handler&& handler, int state = 0, boost::asio::ip::tcp::resolver::iterator i = {})
    {
        using std::move;
        using boost::system::error_code;
        using iterator = boost::asio::ip::tcp::resolver::iterator;

        switch (state)
        {
        case 0:
            BOOST_ASSERT(query_);
            resolver_.async_resolve(*query_, [this, handler=move(handler)](error_code const& ec, iterator i)
            {
                if (ec) return handler(ec);
                this->async_connect(handler, 1, i);
            });
            break;

        case 1:
            boost::asio::async_connect(socket_, i, [this, handler=move(handler)](error_code const& ec, iterator i)
            {
                if (ec) return handler(ec);
                this->async_connect(handler, 2, i);
            });
            break;

        case 2:
            handler();
            break;
        }
    }

    boost::asio::ip::tcp::socket& socket() { return socket_; }
};

}}} // namespace network { namespace protocols { namespace http {
