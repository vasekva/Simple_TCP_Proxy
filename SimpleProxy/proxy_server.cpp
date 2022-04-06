#include "main_header.hpp"

namespace tcp_proxy
{
	bridge::bridge(boost::asio::io_service &ios)
	: server_socket(ios), client_socket(ios)
	{}

	typename bridge::t_socket &bridge::get_server_socket() { return server_socket; }
	typename bridge::t_socket &bridge::get_client_socket() { return client_socket; }

	void bridge::start(const std::string &client_host, unsigned short client_port)
	{
		// Attempt connection to remote server (client side)
		client_socket.async_connect(
				ip::tcp::endpoint(boost::asio::ip::address::from_string(client_host), client_port),
				boost::bind(&bridge::handle_client_connect, shared_from_this(), boost::asio::placeholders::error));
	}
	void bridge::handle_client_connect(const boost::system::error_code &error)
	{
		if (!error)
		{
			// Setup async read from remote server (client side)
			client_socket.async_read_some(
				boost::asio::buffer(client_data, max_data_length),
				boost::bind(&bridge::handle_client_read, shared_from_this(),
					boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

			// Setup async read from server (server side)
			server_socket.async_read_some(
				boost::asio::buffer(server_data, max_data_length),
				boost::bind(&bridge::handle_server_read, shared_from_this(),
					boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
		}
		else
			close();
	}

	/*
	========================================================
	==             Client --> Proxy --> Server            ==
	==                                                    ==
	==          Process data recieved from client         ==
	==              then write to server.                 ==
	========================================================
	*/
    void bridge::handle_client_read(const boost::system::error_code &error,
		const size_t &bytes_transferred)
	{
		if (!error)
		{
			async_write(server_socket,
				boost::asio::buffer(client_data, bytes_transferred),
				boost::bind(&bridge::handle_server_write,
				shared_from_this(),
				boost::asio::placeholders::error));
		}
		else
			close();
	}

    
    void bridge::handle_server_write(const boost::system::error_code &error)
    {
    	if (!error)
        {
           client_socket.async_read_some(
                boost::asio::buffer(client_data, max_data_length),
                boost::bind(&bridge::handle_client_read,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
            close();
	}

	/*
	========================================================
	==             Server --> Proxy --> Client            ==
	==                                                    ==
	==          Process data recieved from server         ==
	==               then write to client.                ==
	========================================================
	*/

	void bridge::handle_server_read(const boost::system::error_code &error,
		const size_t &bytes_transferred)
	{
		if (!error)
		{
			async_write(client_socket,
				boost::asio::buffer(server_data, bytes_transferred),
				boost::bind(&bridge::handle_client_write,
				shared_from_this(),
				boost::asio::placeholders::error));
	}
	else
		close();

	}

    void bridge::handle_client_write(const boost::system::error_code &error)
    {
        if (!error)
        {
            server_socket.async_read_some(
                boost::asio::buffer(server_data, max_data_length),
                boost::bind(&bridge::handle_server_read,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
            close();
    }

    void bridge::close()
    {
    	boost::mutex::scoped_lock lock(t_mutex);
    	if (server_socket.is_open()) { server_socket.close(); }
    	if (client_socket.is_open()) { client_socket.close(); }
    }

	bridge::acceptor::acceptor(boost::asio::io_service &io_service,
		const string &local_host, unsigned short local_port,
		const string &client_host, unsigned short client_port)
			: t_io_service(io_service),
			localhost(boost::asio::ip::address_v4::from_string(local_host)),
			t_acceptor(t_io_service, ip::tcp::endpoint(localhost, local_port)),
			client_port(client_port),
			client_host(client_host)
	{}

	bool bridge::acceptor::accept_connections()
	{
		try
		{
			session_ = boost::shared_ptr<bridge>(new bridge(t_io_service));
			t_acceptor.async_accept(session_->get_server_socket(), boost::bind(&acceptor::handle_accept,
				this, boost::asio::placeholders::error));
		}
		catch (std::exception &ex)
		{
			std::cerr << "acceptor exception: " << ex.what() << std::endl;
			return false;
		}
		return true;
	}

	void bridge::acceptor::handle_accept(const boost::system::error_code &error)
	{
		if (!error)
		{
			session_->start(client_host, client_port);
			if (!accept_connections())
				cerr << "Failure during call to accept." << endl;
		}
		else
			cerr << "Error: " << error.message() << endl;
	}
}