// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  streaming_server.hpp - MAME Game Streaming Server (aka DinoServer 🦕🧡🦖)
//
//============================================================
#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include <iostream>
#include <functional>
#include <memory>

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"


namespace webpp
{
	class streaming_server
	{
	private:
		bool active = false;
		std::unique_ptr<ws_server> server;

	public:
		std::function<void()> on_accept;

	private:
		streaming_server() = default;
		~streaming_server() = default;
		streaming_server(const streaming_server&) = delete;
		streaming_server& operator=(const streaming_server&) = delete;


	public:
		static streaming_server& get()
		{
			static streaming_server instance;
			return instance;
		}

		bool isActive() const
		{
			return active;
		}

		void send()
		{
			auto stream = std::make_shared<ws_server::SendStream>();
			*stream << "Sto cazzo!";

			for (auto c : server->get_connections()) {
				server->send(c, stream, [](auto err) {
					std::cout << "Errore " << err << std::endl;
				});
			}
		}

		void send(std::shared_ptr<ws_server::SendStream> stream)
		{
			///fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.

			for (auto c : server->get_connections()) {
				server->send(c, stream, [](auto err) {
					std::cout << "Errore " << err << std::endl;
				});
			}
		}

		std::shared_ptr<ws_server::SendStream> getStream()
		{
			return std::make_shared<ws_server::SendStream>();
		}

		void start(unsigned short port)
		{
			server = std::make_unique<ws_server>();
			server->config.client_mode = true;
			server->config.port = port;

			auto& endpoint = server->m_endpoint["/"];

			endpoint.on_open = [&](auto connection) {
				std::cout
					<< "-Opened connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				on_accept();
			};

			endpoint.on_message = [](auto connection, auto message) {
				// input handling
			};

			endpoint.on_close = [](auto connection, auto status, auto reason) {
				std::cout
					<< "-Closed connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl
					<< ": " << reason
					<< std::endl;

				// gestire chiusura forzata
			};

			std::cout
				<< "Game streming server listening on "
				<< port
				<< std::endl;

			server->start();
		}
	};
} // namespace webpp

#endif //SRC_STREAMINGSERVER_H
