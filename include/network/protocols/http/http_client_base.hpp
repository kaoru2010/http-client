#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>

namespace network { namespace protocols { namespace http {

class http_client_base;
using http_client_ptr = std::shared_ptr<http_client_base>;

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
