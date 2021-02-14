// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  streaming_server.hpp - MAME Game Streaming Server (aka DinoServer)
//
//============================================================
#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include <list>
#include <map>
#include <iostream>
#include <functional>
#include <memory>
#include <thread>

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"


namespace webpp
{
	class streaming_server
	{
	private:
		bool active = false;
		webpp::ws_server server;
		std::map<std::thread::id, std::shared_ptr<Connection>> connections;
		std::map<std::thread::id, std::shared_ptr<std::thread>> threads;

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

		std::shared_ptr<Connection> get_connection(std::thread::id id)
		{
			return connections[id];
		}

		void send(std::thread::id id, std::shared_ptr<webpp::ws_server::SendStream> stream)
		{
			send(get_connection(id), stream);
		}

		void send(std::shared_ptr<Connection> connection, std::shared_ptr<webpp::ws_server::SendStream> stream)
		{
			server.send(connection, stream);
		}

		std::shared_ptr<webpp::ws_server::SendStream> getStream()
		{
			return std::make_shared<webpp::ws_server::SendStream>();
		}

		void start(short port)
		{
			server.config.port = port;

			auto& endpoint = server.m_endpoint["/"];

			endpoint.on_open = [&](std::shared_ptr<Connection> connection) {
				std::cout
					<< "-Opened connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				auto t = std::make_shared<std::thread>(std::thread(on_accept));
				t->detach();

				auto id = t->get_id();

				threads[id] = t;
				connections[id] = connection;
			};

			endpoint.on_message = [](auto connection, auto message) {
				// input handling
			};

			endpoint.on_close = [&](std::shared_ptr<Connection> connection, int status, const std::string& reason) {
				std::cout
					<< "-Closed connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl
					<< ": " << reason
					<< std::endl;
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
