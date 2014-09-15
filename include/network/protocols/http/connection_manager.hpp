#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <memory>
#include <boost/asio.hpp>

namespace network { namespace protocols { namespace http {

class connection_manager
{
    std::set<http_client_ptr> http_clients_;

public:
    connection_manager()
    {}

    void stop(http_client_ptr ptr)
    {
        http_clients_.erase(ptr);
        ptr->stop();
    }

    void stop_all()
    {
        for (auto&& ptr : http_clients_) {
            ptr->stop();
        }
        http_clients_.clear();
    }

    void start(http_client_ptr ptr)
    {
        http_clients_.insert(ptr);
        ptr->set_connection_manager(this);
        ptr->start();
    }
};

}}} // namespace network { namespace protocols { namespace http {

