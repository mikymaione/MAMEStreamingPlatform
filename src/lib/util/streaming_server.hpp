// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  streaming_server.hpp - MAME Game Streaming Server (aka DinoServer 🦕🧡🦖)
//
//============================================================
#pragma once

#include <ios>
#include <iostream>
#include <sstream>
#include <functional>
#include <memory>
#include <thread>
#include <string>

// standard SDL headers
#include <SDL2/SDL.h>

#include "server_ws_impl.hpp"
#include "server_http_impl.hpp"
#include "encoding/encode_to_mp4.hpp"

namespace webpp
{
	class streaming_server
	{
	private:
		bool active = true;
		std::unique_ptr<ws_server> server;
		std::unique_ptr<std::thread> acceptThread;

		std::unique_ptr<encoding::encode_to_mp4> encoder = std::make_unique<encoding::encode_to_mp4>();

		int64_t append_count_send = 5;
		int64_t append_count = 0;
		std::shared_ptr<ws_server::SendStream> append_stream = std::make_shared<ws_server::SendStream>();

	public:
		std::function<void()> on_accept;

	private:
		streaming_server() = default;
		~streaming_server() = default;
		streaming_server(const streaming_server &) = delete;
		streaming_server &operator=(const streaming_server &) = delete;

	public:
		static streaming_server &get()
		{
			static streaming_server instance;
			return instance;
		}

		bool isActive() const
		{
			return active;
		}

		///fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
		void send(std::shared_ptr<ws_server::SendStream> stream, unsigned char fin_rsv_opcode)
		{
			for (auto c : server->get_connections())
				server->send(c, stream, nullptr, fin_rsv_opcode);
		}

		void send_string(const std::string &msg)
		{
			auto stream = std::make_shared<ws_server::SendStream>();
			*stream << msg;

			send(stream, 129);
		}

		void send_binary(char *b, std::streamsize len)
		{
			auto stream = std::make_shared<ws_server::SendStream>();
			stream->write(b, len);

			send(stream, 130);
		}

		void append_SDL_Surface(SDL_Surface *surf)
		{
			append_count++;
			encoder->encode_frame(append_count, surf, append_stream);

			if (append_count == append_count_send)
			{
				append_count = 0;
				send(append_stream, 130);

				append_stream->clear();
			}
		}

		void start(unsigned short port)
		{
			server = std::make_unique<ws_server>();
			server->config.client_mode = true;
			server->config.port = port;

			auto &endpoint = server->m_endpoint["/"];

			endpoint.on_open = [&](auto connection) {
				std::cout
					<< "-Opened connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				acceptThread = std::make_unique<std::thread>(on_accept);
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