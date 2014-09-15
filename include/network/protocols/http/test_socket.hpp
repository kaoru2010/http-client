#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>

#include <network/protocols/http/test_stream.hpp>

namespace network { namespace protocols { namespace http {

class test_socket
{
public:
    typedef boost::asio::io_service io_service_type;

    test_socket(boost::asio::io_service& io_service)
    :   io_service_(io_service)
    ,   stream_(io_service)
    {
    }

    test_stream& socket()
    {
        return stream_;
    }

    template <typename Socket, typename Handler>
    void async_connect(Socket& socket, Handler&& handler)
    {
        io_service_.post([handler = std::move(handler)]()
        {
            handler();
        });
    }

private:
    io_service_type& io_service_;
    test_stream stream_;
};

}}} // namespace network { namespace protocols { namespace http {
