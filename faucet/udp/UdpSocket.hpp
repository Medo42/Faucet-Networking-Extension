#pragma once

#include "DatagramQueue.hpp"

#include <faucet/Socket.hpp>
#include <faucet/ReadWritable.hpp>
#include <faucet/Asio.hpp>
#include <faucet/Buffer.hpp>
#include <faucet/V4FirstIterator.hpp>

#include <boost/integer.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/utility.hpp>
#include <string>
#include <memory>

// TODO: Seperate error reporting for ipv4 and ipv6
class UdpSocket: public Socket,
		public std::enable_shared_from_this<UdpSocket>,
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
	virtual void setReadpos(size_t pos);

	virtual size_t getSendbufferSize();
	virtual size_t getReceivebufferSize();
	virtual void setSendbufferLimit(size_t maxSize);

	virtual Buffer &getReceiveBuffer();

	virtual std::string getRemoteIp();
	virtual uint16_t getRemotePort();
	virtual uint16_t getLocalPort();

	bool send(const std::string &host, uint16_t port);
	bool broadcast(uint16_t port);
	bool receive();

	void close();

	static std::shared_ptr<UdpSocket> error(const std::string &message);
	static std::shared_ptr<UdpSocket> bind(uint16_t port);

private:
	UdpSocket();
	void asyncSend();
	void handleSend(const boost::system::error_code &err, std::shared_ptr<Buffer> sendBuffer, V4FirstIterator<boost::asio::ip::udp> endpoints);
	void asyncReceive(boost::asio::ip::udp::socket *sock, std::shared_ptr<std::array<uint8_t, 65536>> recvbuffer);
	static void handleReceive(std::weak_ptr<UdpSocket> ptr, const boost::system::error_code &err,
			size_t bytesTransferred, std::shared_ptr<boost::asio::ip::udp::endpoint> endpoint,
			boost::asio::ip::udp::socket *sock, std::shared_ptr<std::array<uint8_t, 65536>> recvbuffer);
	void handleResolve(const boost::system::error_code &error,
			boost::asio::ip::udp::resolver::iterator endpointIterator,
			std::shared_ptr<Buffer> buffer);
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

	std::shared_ptr<Buffer> receiveBuffer_;
	std::shared_ptr<Buffer> sendBuffer_;
};
