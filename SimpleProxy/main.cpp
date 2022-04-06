#include "main_header.hpp"

/**
 Server --> Proxy --> Client
    |                    |
 Server <-- Proxy <-- Client
*/

int main(int argc, char **argv)
{
	if (argc != 5)
	{
		std::cerr <<
			"You needed to put 4 additional arguments: "  << 
			"server_ip server_port client_ip client_port" <<
			std::endl;
		return 1;
	}

	const std::string        server_ip = argv[1];
	const unsigned short     server_port = static_cast<unsigned short>(::atoi(argv[2]));
	const std::string        client_ip = argv[3];
	const unsigned short     client_port = static_cast<unsigned short>(::atoi(argv[4]));

	boost::asio::io_service ios;

	try
	{
		tcp_proxy::bridge::acceptor acceptor(ios, server_ip, server_port, client_ip, client_port);
		acceptor.accept_connections();
		ios.run();
	}
	catch (std::exception &ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}