#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include <cstring>
#include <vector>

#include <network/protocols/http/response_parser.hpp>

using namespace std;

class test_stream
{
public:
  typedef boost::asio::io_service io_service_type;

  test_stream(boost::asio::io_service& io_service)
    : io_service_(io_service),
      length_(0),
      position_(0),
      next_read_length_(0)
  {
  }

  io_service_type& get_io_service()
  {
    return io_service_;
  }

  void reset(std::string const& str)
  {
    data_.assign(str.begin(), str.end());

    length_ = str.size();
    position_ = 0;
    next_read_length_ = str.size();
  }

  void set_next_read_length(size_t length)
  {
    next_read_length_ = length;
  }

  template <typename Mutable_Buffers>
  size_t read_some(const Mutable_Buffers& buffers)
  {
    size_t n = boost::asio::buffer_copy(buffers,
        boost::asio::buffer(data_, length_) + position_,
        next_read_length_);
    position_ += n;
    return n;
  }

  template <typename Mutable_Buffers>
  size_t read_some(const Mutable_Buffers& buffers,
      boost::system::error_code& ec)
  {
    ec = boost::system::error_code();
    return read_some(buffers);
  }

  template <typename Mutable_Buffers, typename Handler>
  void async_read_some(const Mutable_Buffers& buffers, Handler handler)
  {
    size_t bytes_transferred = read_some(buffers);
    io_service_.post(boost::asio::detail::bind_handler(
          handler, boost::system::error_code(), bytes_transferred));
  }

private:
  io_service_type& io_service_;
  vector<char> data_;
  size_t length_;
  size_t position_;
  size_t next_read_length_;
};

BOOST_AUTO_TEST_SUITE(http_request_test)

BOOST_AUTO_TEST_CASE(async_read)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset("GET");

    char buf[100];
    boost::asio::async_read(s, boost::asio::buffer(buf), [&](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        BOOST_CHECK_EQUAL(3, bytes_transferred);
    });

    io_service.run();

    BOOST_CHECK_EQUAL("GET", string(buf, 3));
}

#define TEST_HEADER "HTTP/1.0 200 OK\r\n"   \
    "Content-Type: text/plain\r\n"          \
    "Content-Length: 7\r\n"                 \
    "\r\n"

BOOST_AUTO_TEST_CASE(async_read_until)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset(TEST_HEADER "*BODY*\n");

    boost::asio::streambuf buf;
    size_t size = 0;
    boost::asio::async_read_until(s, buf, "\r\n\r\n", [&](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        size = bytes_transferred;
        BOOST_CHECK_EQUAL(strlen(TEST_HEADER), bytes_transferred);
    });

    io_service.run();

    BOOST_CHECK_EQUAL(TEST_HEADER, string(boost::asio::buffer_cast<const char *>(buf.data()), size));
}

BOOST_AUTO_TEST_CASE(async_read_header)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset(TEST_HEADER "*BODY*\n");

    boost::asio::streambuf buf;
    size_t size = 0;
    network::protocols::http::http_response_context ctx;
    network::protocols::http::async_read_header(s, buf, ctx, [&](const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        size = bytes_transferred;
    });

    io_service.run();

    BOOST_CHECK_EQUAL(strlen(TEST_HEADER), size);
    BOOST_CHECK_EQUAL("*BODY*\n", string(boost::asio::buffer_cast<const char *>(buf.data()), boost::asio::buffer_size(buf.data())));
    BOOST_CHECK_EQUAL(200, ctx.status_code);
    BOOST_CHECK_EQUAL(7, *ctx.content_length);
    BOOST_CHECK_EQUAL("text/plain", ctx.content_type ? *ctx.content_type : "**none**");
}

BOOST_AUTO_TEST_CASE(parse_response_header)
{
    char buf[] = TEST_HEADER "*BODY*\n";
    network::protocols::http::http_response_context ctx;
    network::protocols::http::parse_response_header(buf, strlen(buf), ctx);
    BOOST_CHECK_EQUAL(200, ctx.status_code);
    BOOST_CHECK_EQUAL(7, *ctx.content_length);
    BOOST_CHECK_EQUAL("text/plain", ctx.content_type ? *ctx.content_type : "**none**");
}

#undef TEST_HEADER


#define TEST_TRAILER "Key: Value\r\n" \
    "Key: Value\r\n" \
    "\r\n"

BOOST_AUTO_TEST_CASE(async_read_trailer)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset(TEST_TRAILER "**MARKER**");

    boost::asio::streambuf buf;
    network::protocols::http::async_read_trailer(s, buf, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });

    io_service.run();

    BOOST_CHECK_EQUAL("**MARKER**", string(boost::asio::buffer_cast<const char *>(buf.data()), boost::asio::buffer_size(buf.data())));
}

#undef TEST_TRAILER


#define TEST_CHUNK "2\r\n"  \
    "OK\r\n"                \
    "10;key=value\r\n"       \
    "0123456789abcdef\r\n"  \
    "0\r\n"                 \
    "trailer: val\r\n"      \
    "\r\n"

BOOST_AUTO_TEST_CASE(async_read_chunk)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset(TEST_CHUNK "**MARKER**");

    boost::asio::streambuf buf;
    size_t chunk_size = 0;

    network::protocols::http::async_read_chunk_size(s, buf, chunk_size, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });
    io_service.run();
    BOOST_CHECK_EQUAL(2, chunk_size);

    network::protocols::http::async_read_chunk_data(s, buf, chunk_size, [&](const boost::system::error_code& ec, size_t size)
    {
        BOOST_CHECK_EQUAL(2, size);
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
        BOOST_CHECK_EQUAL("OK", string(boost::asio::buffer_cast<const char *>(buf.data()), size));
        buf.consume(size);
    });
    io_service.reset();
    io_service.run();

    network::protocols::http::async_read_chunk_size(s, buf, chunk_size, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });
    io_service.reset();
    io_service.run();
    BOOST_CHECK_EQUAL(16, chunk_size);

    network::protocols::http::async_read_chunk_data(s, buf, chunk_size, [&](const boost::system::error_code& ec, size_t size)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
        BOOST_CHECK_EQUAL("0123456789abcdef", string(boost::asio::buffer_cast<const char *>(buf.data()), size));
        buf.consume(size);
    });
    io_service.reset();
    io_service.run();

    network::protocols::http::async_read_chunk_size(s, buf, chunk_size, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });
    io_service.reset();
    io_service.run();
    BOOST_CHECK_EQUAL(0, chunk_size);

    network::protocols::http::async_read_trailer(s, buf, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });
    io_service.reset();
    io_service.run();

    BOOST_CHECK_EQUAL("**MARKER**", string(boost::asio::buffer_cast<const char *>(buf.data()), boost::asio::buffer_size(buf.data())));
}

#undef TEST_CHUNK

BOOST_AUTO_TEST_CASE(async_read_chunk_size)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset("ff\r\n");

    boost::asio::streambuf buf;
    size_t chunk_size = 0;

    network::protocols::http::async_read_chunk_size(s, buf, chunk_size, [&](const boost::system::error_code& ec)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
    });
    io_service.run();
    BOOST_CHECK_EQUAL(255, chunk_size);
}

BOOST_AUTO_TEST_CASE(async_read_until_with_size)
{
    boost::asio::io_service io_service;

    test_stream s(io_service);
    s.reset("12");

    bool called = false;
    boost::asio::streambuf buf;
    network::protocols::http::async_read_until_with_size(s, buf, 1, [&](const boost::system::error_code& ec, size_t size)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
        BOOST_CHECK_EQUAL(1, size);
        BOOST_CHECK_EQUAL("1", string(boost::asio::buffer_cast<const char *>(buf.data()), size));
        called = true;
        buf.consume(size);
    });
    io_service.run();
    BOOST_CHECK_EQUAL(true, called);

    io_service.reset();
    called = false;
    network::protocols::http::async_read_until_with_size(s, buf, 1, [&](const boost::system::error_code& ec, size_t size)
    {
        BOOST_CHECK_EQUAL(boost::system::error_code(), ec);
        BOOST_CHECK_EQUAL(1, size);
        BOOST_CHECK_EQUAL("2", string(boost::asio::buffer_cast<const char *>(buf.data()), size));
        called = true;
        buf.consume(size);
    });
    io_service.run();
    BOOST_CHECK_EQUAL(true, called);
}

BOOST_AUTO_TEST_SUITE_END()
