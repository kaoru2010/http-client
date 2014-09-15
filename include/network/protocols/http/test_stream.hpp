#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>

namespace network { namespace protocols { namespace http {

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
  std::vector<char> data_;
  size_t length_;
  size_t position_;
  size_t next_read_length_;
};

}}} // namespace network { namespace protocols { namespace http {
