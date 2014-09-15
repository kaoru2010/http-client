#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <sstream>

#include <cstring>
#include <vector>

#include <network/protocols/http/http_request.hpp>

using namespace std;
using namespace network::protocols::http;

BOOST_AUTO_TEST_SUITE(http_request)

BOOST_AUTO_TEST_CASE(GET)
{
    http_get_request req("GET", "/", "localhost");
    BOOST_CHECK_EQUAL("GET", req.method());
    BOOST_CHECK_EQUAL("/", req.uri());
    BOOST_CHECK_EQUAL("localhost", req.hostname());
}

BOOST_AUTO_TEST_CASE(POST)
{
    http_post_request req("POST", "/", "localhost");
    BOOST_CHECK_EQUAL("POST", req.method());
    BOOST_CHECK_EQUAL("/", req.uri());
    BOOST_CHECK_EQUAL("localhost", req.hostname());
}

#define REQUEST_TEXT \
    "GET / HTTP/1.1\r\n" \
    "Host: localhost\r\n" \
    "\r\n"

BOOST_AUTO_TEST_CASE(GET_request)
{
    stringstream ss;
    http_get_request req("GET", "/", "localhost");
    ss << req;

    BOOST_CHECK_EQUAL(REQUEST_TEXT, ss.str());
}

#undef REQUEST_TEXT

#define REQUEST_TEXT \
    "POST / HTTP/1.1\r\n" \
    "Host: localhost\r\n" \
    "Content-Length: 0\r\n" \
    "Content-Type: application/x-www-form-urlencoded\r\n" \
    "\r\n"

BOOST_AUTO_TEST_CASE(POST_request)
{
    stringstream ss;
    http_post_request req("POST", "/", "localhost");
    ss << req;

    BOOST_CHECK_EQUAL(REQUEST_TEXT, ss.str());
}

#undef REQUEST_TEXT

BOOST_AUTO_TEST_SUITE_END()
