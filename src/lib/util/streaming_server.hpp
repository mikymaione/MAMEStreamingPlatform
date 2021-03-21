// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  streaming_server.hpp - MAME Game Streaming Server (aka DinoServer 🦕🧡🦖)
//
//============================================================
#pragma once

#include <iostream>
#include <functional>
#include <memory>
#include <thread>
#include <string>

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

		std::unique_ptr<encoding::encode_to_mp4> encoder
			= std::make_unique<encoding::encode_to_mp4>(640, 480, 640, 480, 4, 15);

	public:
		/**
		 * \brief Accept callback
		 */
		std::function<void()> on_accept;
		/**
		 * \brief Connection closed callback
		 */
		std::function<void()> on_connection_closed;

	public:
		streaming_server() = default;
		~streaming_server() = default;
		streaming_server(const streaming_server&) = delete;
		streaming_server& operator=(const streaming_server&) = delete;

	private:
		///fin_rsv_operation_code: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
		void send(const std::shared_ptr<ws_server::SendStream>& stream, const unsigned char fin_rsv_operation_code) const
		{
			for (const auto& c : server->get_connections())
				server->send(c, stream, nullptr, fin_rsv_operation_code);
		}

	public:
		/**
		 * \brief Return the singleton instance of the class
		 * \return the instance
		 */
		static streaming_server& get()
		{
			static streaming_server instance;
			return instance;
		}

		bool is_active() const
		{
			return active;
		}

		/**
		 * \brief Send a string to the client
		 * \param msg
		 */
		void send_string(const std::string& msg) const
		{
			const auto stream = std::make_shared<ws_server::SendStream>();
			*stream << msg;

			send(stream, 129);
		}

		/**
		 * \brief Send video frame to client
		 * \param pixels
		 */
		void send_video_frame(uint8_t* pixels) const
		{
			const auto stream = std::make_shared<ws_server::SendStream>();

			if (encoder->add_frame(pixels, stream))
				send(stream, 130);
		}

		/**
		 * \brief Send audio interval to client
		 * \param audio_stream
		 * \param in_sample_rate
		 * \param samples
		 */
		void send_audio_interval(uint8_t* audio_stream, int in_sample_rate, int samples) const
		{
			const auto stream = std::make_shared<ws_server::SendStream>();

			if (encoder->add_instant(audio_stream, in_sample_rate, samples, stream))
				send(stream, 130);
		}

		void start(const unsigned short port)
		{
			server = std::make_unique<ws_server>();
			server->config.client_mode = true;
			server->config.port = port;

			auto& endpoint = server->m_endpoint["/"];

			endpoint.on_open = [&](auto connection)
			{
				std::cout
					<< "-Opened connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				acceptThread = std::make_unique<std::thread>(on_accept);
			};

			endpoint.on_message = [](auto connection, auto message)
			{
				// input handling
			};

			endpoint.on_close = [&](auto connection, auto status, auto reason)
			{
				std::cout
					<< "-Closed connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl
					<< ": " << reason
					<< std::endl;

				on_connection_closed();
			};

			std::cout
				<< "Game streaming server listening on "
				<< port
				<< std::endl;

			server->start();
		}

	};
} // namespace webpp
