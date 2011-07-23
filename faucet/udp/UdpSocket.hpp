#pragma once

#include "DatagramQueue.hpp"

#include <faucet/Socket.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/Asio.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/V4FirstIterator.hpp>

#include <boost/integer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/utility.hpp>
#include <string>

// TODO: Seperate error reporting for ipv4 and ipv6
class UdpSocket: public Socket,
		public boost::enable_shared_from_this<UdpSocket>,
		boost::noncopyable {

public:
	virtual ~UdpSocket();

	virtual std::string getErrorMessage();
	virtual bool hasError();

	// Functions required by the ReadWritable interface
	virtual void write(const uint8_t *in, size_t size);
	virtual size_t read(uint8_t *out, size_t size);
	virtual std::string readString(size_t size);
	virtual size_t bytesRemaining() const;

	virtual size_t getSendbufferSize();
	virtual size_t getReceivebufferSize();
	virtual void setSendbufferLimit(size_t maxSize);

	virtual Buffer &getReceiveBuffer();

	virtual std::string getRemoteIp();
	virtual uint16_t getRemotePort();
	virtual uint16_t getLocalPort();

	bool send(const std::string &host, uint16_t port);
	bool receive();

	void close();

	static boost::shared_ptr<UdpSocket> error(const std::string &message);
	static boost::shared_ptr<UdpSocket> bind(uint16_t port);

private:
	UdpSocket();
	void asyncSend();
	void handleSend(const boost::system::error_code &err, boost::shared_ptr<Buffer> sendBuffer, V4FirstIterator<boost::asio::ip::udp> endpoints);
	void asyncReceive(boost::asio::ip::udp::socket *sock, boost::shared_array<uint8_t> recvbuffer);
	static void handleReceive(boost::weak_ptr<UdpSocket> ptr, const boost::system::error_code &err,
			size_t bytesTransferred, boost::shared_ptr<boost::asio::ip::udp::endpoint> endpoint,
			boost::asio::ip::udp::socket *sock, boost::shared_array<uint8_t> recvbuffer);
	void handleResolve(const boost::system::error_code &error,
			boost::asio::ip::udp::resolver::iterator endpointIterator,
			boost::shared_ptr<Buffer> buffer);
	boost::asio::ip::udp::socket *getAppropriateSocket(
			const boost::asio::ip::udp::endpoint &endpoint);

	boost::recursive_mutex commonMutex_;

	DatagramQueue sendqueue_;
	DatagramQueue receivequeue_;

	bool asyncSendInProgress_;

	boost::asio::ip::udp::socket ipv4socket_;
	boost::asio::ip::udp::socket ipv6socket_;
	boost::asio::ip::udp::resolver resolver_;

	bool hasError_;
	std::string errorMessage_;

	uint16_t localPort_;
	std::string remoteIp_;
	uint16_t remotePort_;

	boost::shared_ptr<Buffer> receiveBuffer_;
	boost::shared_ptr<Buffer> sendBuffer_;
};
