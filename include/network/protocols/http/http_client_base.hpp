#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>

namespace network { namespace protocols { namespace http {

class http_client_base
{
public:
    virtual ~http_client_base() = 0;
};

inline
http_client_base::~http_client_base()
{
}

}}} // namespace network { namespace protocols { namespace http {
