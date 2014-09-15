#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include <cstring>
#include <vector>

#include <network/protocols/http/http_client.hpp>
#include <network/protocols/http/test_socket.hpp>

using namespace std;

struct request_handler
{
    template <typename Socket, typename Handler>
    void async_request(Socket& socket, Handler&& handler)
    {
        socket.get_io_service().post([handler = std::move(handler)]()
        {
            handler();
        });
    }
};

struct response_handler {};

BOOST_AUTO_TEST_SUITE(http_client)

BOOST_AUTO_TEST_CASE(constructor)
{
    boost::asio::io_service io_service;

    using http_client = network::protocols::http::http_client<
        network::protocols::http::test_socket, request_handler, response_handler>;

    auto client = make_shared<http_client>(io_service);
    client->start();

    io_service.run();
    BOOST_CHECK_EQUAL(true, true);
}

BOOST_AUTO_TEST_SUITE_END()
