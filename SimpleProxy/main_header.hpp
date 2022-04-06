#ifndef MAIN_HEADER_HPP
#define MAIN_HEADER_HPP

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

namespace tcp_proxy
{
	using std::string;
	using std::cerr;
	using std::endl;

	namespace ip = boost::asio::ip;

	class bridge : public boost::enable_shared_from_this<bridge>
	{
		public:
			typedef boost::asio::ip::tcp::socket    t_socket;
			typedef boost::shared_ptr<bridge>       t_ptr;

			bridge(boost::asio::io_service &ios);

			t_socket &get_server_socket();
			t_socket &get_client_socket();

			void start(const string &client_host, unsigned short client_port);
			void handle_client_connect(const boost::system::error_code &error);

		private:
			// Read from remote server complete, now send data to client
			void handle_client_read(const boost::system::error_code &error,
				const size_t &bytes_transferred);
			// Write to remote server complete, Async read from client
			void handle_client_write(const boost::system::error_code &error);
			// Read from client complete, now send data to remote server
			void handle_server_read(const boost::system::error_code &error,
				const size_t &bytes_transferred);
			// Write to client complete, Async read from remote server
			void handle_server_write(const boost::system::error_code &error);
			void close();

			t_socket server_socket;
			t_socket client_socket;

			enum { max_data_length = 8192 }; //8KB
			unsigned char server_data[max_data_length];
			unsigned char client_data[max_data_length];

			boost::mutex t_mutex;

		public:
			class acceptor
			{
				public:
					acceptor(boost::asio::io_service &io_service,
							 const string &local_host, unsigned short local_port,
							 const string &client_host, unsigned short client_port);

					bool accept_connections();

				private:
					void handle_accept(const boost::system::error_code &error);

					boost::asio::io_service &t_io_service;
					ip::address_v4          localhost;
					ip::tcp::acceptor       t_acceptor;
					t_ptr                   session_;
					unsigned short          client_port;
					string                  client_host;
			};
	};
}

#endif
