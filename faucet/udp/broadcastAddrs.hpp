#pragma once

#include<vector>
#include<boost/asio.hpp>

std::vector<boost::asio::ip::address_v4> findLocalBroadcastAddresses();
