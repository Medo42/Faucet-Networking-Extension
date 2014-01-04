#include <faucet/Asio.hpp>
#include <stdexcept>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

void Asio::startup() {
	if(ioService != 0) {
		return;
	}
	ioService = new boost::asio::io_service();
	work = new boost::asio::io_service::work(*ioService);
	butler = new boost::thread(boost::bind(&boost::asio::io_service::run, ioService));
}

boost::asio::io_service &Asio::getIoService() {
	if(ioService == 0) {
		throw std::runtime_error("Attempted to access io_service before startup or after shutdown.");
	}
	return *ioService;
}

void Asio::shutdown() {
	if(ioService == 0) {
		return;
	}
	delete work;

	ioService->stop();
	butler->join();
	delete butler;
	delete ioService;

	work = 0;
	butler = 0;
	ioService = 0;
}

boost::asio::io_service *Asio::ioService = 0;
boost::asio::io_service::work *Asio::work = 0;
boost::thread *Asio::butler = 0;
