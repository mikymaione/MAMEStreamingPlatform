#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"

#include <memory>

namespace webpp
{
	class StreamingServer
	{
	private:
		std::shared_ptr<asio::io_context>   m_io_context;
		std::unique_ptr<webpp::http_server>	m_server;
		std::unique_ptr<webpp::ws_server>	m_wsserver;
		std::thread							m_server_thread;

	public:
		StreamingServer(short port)
		{
			m_io_context = std::make_shared<asio::io_context>();

			m_server = std::make_unique<webpp::http_server>();
			m_server->m_config.port = port;
			m_server->set_io_context(m_io_context);

			m_wsserver = std::make_unique<webpp::ws_server>();

			auto& endpoint = m_wsserver->m_endpoint["/"];

			endpoint.on_open = [&](auto connection)
			{
				auto send_stream = std::make_shared<webpp::ws_server::SendStream>();
				*send_stream << "update_machine";
				m_wsserver->send(connection, send_stream);
			};

			m_server->start();

			m_server_thread = std::thread([this]()
				{
					m_io_context->run();
				});
		}
	};
} // namespace webpp

#endif //SRC_STREAMINGSERVER_H
