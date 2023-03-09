/*
 * networking.hpp
 *
 *  Created on: Mar 4, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_NETWORKING_HPP_
#define ALPHAGOMOKU_UTILS_NETWORKING_HPP_

#include <vector>
#include <string>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

namespace ag
{

	class NetworkError: std::runtime_error
	{
		public:
			NetworkError(const std::string &msg);
	};

	class Client
	{
			std::string host_name;
			int port_number = 0;
			int socket_fd = -1;

			struct sockaddr_in serv_addr;
			struct hostent *server;

		public:
			Client(const std::string &host, int port);
			~Client();
			bool connectToServer();
			bool disconnectFromServer();
			bool sendToServer(const std::vector<char> &data);
			std::vector<char> receiveFromServer();
	};

	class Server
	{
			bool hasConnection;

			int portNumber;
			int socket_fd, connection_fd;
			struct sockaddr_in server_address, client_address;
			socklen_t client_length;
		public:
			Server(int port);
			~Server();
			bool bindToPort();
			bool unbindFromPort();
			bool acceptConnection();
			bool closeConnection();
			std::vector<char> receiveFromClient();
			bool sendToClient(const std::vector<char> &data);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_NETWORKING_HPP_ */
