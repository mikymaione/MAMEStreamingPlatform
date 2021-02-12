#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include <functional>
#include <iostream>
#include <list>
#include <thread>

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"


namespace webpp
{
	class StreamingServer
	{
	private:
		webpp::ws_server server;
		std::list<std::thread> threads;

	public:
		std::function<void()> on_accept;


	public:
		void start(short port)
		{
			server.config.port = port;

			auto& endpoint = server.m_endpoint["/"];

			endpoint.on_open = [&](auto connection) {
				//auto send_stream = std::make_shared<webpp::ws_server::SendStream>();
				//*send_stream << "update_machine";
				//server.send(connection, send_stream);

				std::cout
					<< "-Connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				threads.push_back(std::thread(on_accept));
			};

			endpoint.on_message = [](auto connection, auto message) {

			};

			std::cout
				<< "Game streming server listening on "
				<< port
				<< std::endl;

			server.start();
		}

	};
} // namespace webpp

#endif //SRC_STREAMINGSERVER_H
