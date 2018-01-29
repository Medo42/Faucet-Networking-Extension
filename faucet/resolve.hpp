#pragma once

#include <faucet/Asio.hpp>
#include <string>
#include <functional>
#include <boost/lexical_cast.hpp>

enum class fct_lookup_protocol {V4, V6, ANY};

template <typename InternetProtocol>
void fct_async_resolve(std::string host, uint16_t port, typename InternetProtocol::resolver &resolver, std::function<void(const boost::system::error_code&, typename InternetProtocol::resolver::iterator)> handleResolve) {
    fct_async_resolve<InternetProtocol>(host, port, resolver, handleResolve, fct_lookup_protocol::ANY);
}

/**
 * Wrapper around resolver::async_resolve which will first attempt to interpret the hostname as a literal IP address.
 * If it can be parsed this way, the handler function will be called synchronously.
 */
template <typename InternetProtocol>
void fct_async_resolve(std::string host, uint16_t port, typename InternetProtocol::resolver &resolver, std::function<void(const boost::system::error_code&, typename InternetProtocol::resolver::iterator)> handleResolve, fct_lookup_protocol protocol) {
	boost::system::error_code ec;
	typename InternetProtocol::endpoint endpoint(boost::asio::ip::address::from_string(host, ec), port);

	if(!ec) {
        if((protocol == fct_lookup_protocol::V4 && endpoint.address().is_v4()) || (protocol == fct_lookup_protocol::V6 && endpoint.address().is_v6()) || protocol == fct_lookup_protocol::ANY) {
            handleResolve(ec, InternetProtocol::resolver::iterator::create(endpoint, "", ""));
        } else {
            handleResolve(ec, typename InternetProtocol::resolver::iterator());
        }
	} else {
	    std::string portStr(boost::lexical_cast<std::string>(port));
	    typename InternetProtocol::resolver::query::flags flags(InternetProtocol::resolver::query::numeric_service | InternetProtocol::resolver::query::address_configured);
	    typename InternetProtocol::resolver::query query([&](){
	        if(protocol == fct_lookup_protocol::ANY) {
                return typename InternetProtocol::resolver::query(host, portStr, flags);
	        } else {
	            return typename InternetProtocol::resolver::query((protocol == fct_lookup_protocol::V4) ? InternetProtocol::v4() : InternetProtocol::v6(), host, portStr, flags);
	        }
	    }());

        resolver.async_resolve(query, handleResolve);
	}
}
