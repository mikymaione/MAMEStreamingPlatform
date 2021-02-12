#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"


namespace webpp
{
	class StreamingServer
	{
	public:
		StreamingServer(short port)
		{
			webpp::ws_server server;
			server.config.port = port;

			auto& endpoint = server.m_endpoint["/"];

			endpoint.on_open = [&](auto connection) {
				//auto send_stream = std::make_shared<webpp::ws_server::SendStream>();
				//*send_stream << "update_machine";
				//server.send(connection, send_stream);
			};

			endpoint.on_message = [&](auto connection, auto message) {

			};

			server.start();
		}

	};
} // namespace webpp

#endif //SRC_STREAMINGSERVER_H
