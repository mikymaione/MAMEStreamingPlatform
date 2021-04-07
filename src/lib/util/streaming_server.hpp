// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  streaming_server.hpp - MAME Game Streaming Server (aka DinoServer 🦕🧡🦖)
//
//============================================================
#pragma once

#include <cstdlib>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <SDL2/SDL.h>

#include "server_ws_impl.hpp"
#include "encoding/encode_to_mp4.hpp"
#include "modules/input/input_common.h"

namespace webpp
{
	class streaming_server
	{
	private:
		bool active = false;
		unsigned long ping = 0;

		std::chrono::time_point<std::chrono::system_clock> ping_sent =
			std::chrono::system_clock::now();

		std::unique_ptr<ws_server> server;
		std::unique_ptr<std::thread> acceptThread;

		std::unique_ptr<encoding::encode_to_mp4> encoder;

		running_machine* machine = nullptr;
		bool machine_paused_by_server = false;

	public:
		/**
		 * \brief Accept callback
		 */
		std::function<void()> on_accept;
		/**
		 * \brief Connection closed callback
		 */
		std::function<void()> on_connection_closed;

		event_based_device<SDL_Event>* keyboard;

	private:
		///fin_rsv_operation_code: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
		void send(const std::shared_ptr<ws_server::SendStream>& stream, const unsigned char fin_rsv_operation_code) const
		{
			for (const auto& c : server->get_connections())
				server->send(c, stream, nullptr, fin_rsv_operation_code);
		}

		void send_string(const std::string& msg) const
		{
			const auto stream = std::make_shared<ws_server::SendStream>();
			*stream << msg;

			send(stream, 129);
		}

		static std::vector<std::string> split(const std::string s, const std::string del)
		{
			std::vector<std::string> result;

			size_t start = 0;
			auto end = s.find(del);

			while (end != -1)
			{
				result.push_back(s.substr(start, end - start));

				start = end + del.size();
				end = s.find(del, start);
			}

			result.push_back(s.substr(start, end - start));

			return result;
		}

		static bool is_streaming_server(const int argc, char** argv)
		{
			const std::string s = "-streamingserver";

			for (auto i = 1; i < argc; i++)
			{
				auto ok = true;

				for (size_t c = 0; c < s.size(); c++)
					if (s[c] != tolower(argv[i][c]))
					{
						ok = false;
						break;
					}

				if (ok)
					return true;
			}

			return false;
		}

	public:
		/**
		 * \brief Return the singleton instance of the class
		 * \return the instance
		 */
		static streaming_server& instance()
		{
			static streaming_server instance;
			return instance;
		}

		void set_running_machine(running_machine& machine_)
		{
			machine = &machine_;
		}

		bool is_active() const
		{
			return active;
		}

		void activate(const int argc, char** argv)
		{
			active = is_streaming_server(argc, argv);
		}

		static void run_new_process(const int argc, char** argv)
		{
			std::stringstream string_stream;
			string_stream << "start";

			for (auto i = 0; i < argc; ++i)
			{
				string_stream << " ";
				string_stream << argv[i];
			}

			const auto command = string_stream.str();

			system(command.c_str()); //run MAME
		}

		void send_ping()
		{
			const auto now = std::chrono::system_clock::now();
			const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - ping_sent);

			if (machine_paused_by_server && machine->paused() || milliseconds.count() > 2000)
			{
				ping++;
				ping_sent = std::chrono::system_clock::now();

				std::stringstream string_stream;
				string_stream << "ping:" << ping;

				send_string(string_stream.str());
			}
		}

		void set_streaming_input_size(const int w, const int h, const int fps) const
		{
			std::stringstream string_stream;
			string_stream
				<< "size:"
				<< w << ":"
				<< h;

			send_string(string_stream.str());

			encoder->set_streaming_input_size(w, h, fps);
		}

		void set_streaming_output_size(const int w, const int h) const
		{
			encoder->set_streaming_output_size(w, h);
		}

		/**
		* \brief Send video frame to client
		* \param pixels input pixels
		*/
		void send_video_frame(const uint8_t* pixels) const
		{
			encoder->add_frame(pixels);
		}

		/**
		* \brief Add audio instant
		* \param audio_stream input audio stream
		* \param audio_stream_size input audio stream size
		* \param audio_stream_num_samples number of samples
		*/
		void send_audio_interval(const uint8_t* audio_stream, const int audio_stream_size, const int audio_stream_num_samples) const
		{
			encoder->add_instant(audio_stream, audio_stream_size, audio_stream_num_samples);
		}

		void process_key(const char* key, const std::string& down) const
		{
			SDL_Event e;
			e.type = down == "D" ? SDL_KEYDOWN : SDL_KEYUP;
			e.key.keysym.scancode = SDL_GetScancodeFromName(key);
			e.key.keysym.sym = SDL_GetKeyFromScancode(e.key.keysym.scancode);

			keyboard->queue_events(&e, 1);
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

			endpoint.on_message = [&](auto connection, auto message)
			{
				const auto msg = message->string();
				const auto values = split(msg, ":");

				if (values[0] == "ping")
				{
					const auto ping_received = std::chrono::system_clock::now();
					const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ping_received - ping_sent);

					if (milliseconds.count() > 500)
					{
						if (!machine->paused())
						{
							std::cout << "Pausing game!" << std::endl; // rimuovere
							machine_paused_by_server = true;
							machine->pause();
						}
					}
					else
					{
						if (machine_paused_by_server && machine->paused())
						{
							std::cout << "Resuming game!" << std::endl; // rimuovere
							machine_paused_by_server = false;
							machine->resume();
						}
					}
				}
				else if (values[0] == "key")
				{
					const auto down = values[1];
					const auto input_number = values[2];
					const auto key = values[3];

					//https://css-tricks.com/snippets/javascript/javascript-keycodes/
					if (key == "SELECT")process_key("5", down);
					else if (key == "START")process_key("1", down);

					else if (key == "UP")process_key("Up", down);
					else if (key == "DOWN")process_key("Down", down);
					else if (key == "RIGHT")process_key("Right", down);
					else if (key == "LEFT")process_key("Left", down);

					else if (key == "X")process_key("W", down);
					else if (key == "Y")process_key("E", down);
					else if (key == "A")process_key("S", down);
					else if (key == "B")process_key("D", down);

					else if (key == "L1")process_key("Q", down);
					else if (key == "L2")process_key("A", down);
					else if (key == "R1")process_key("R", down);
					else if (key == "R2")process_key("F", down);
				}
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

			const auto socket = std::make_shared<ws_server::SendStream>();

			encoder = std::make_unique<encoding::encode_to_mp4>();

			encoder->socket = socket;
			encoder->on_write = [&]()
			{
				send(socket, 130);
				send_ping();
			};

			std::cout
				<< "Game streaming server listening on "
				<< port
				<< std::endl;

			server->start();
		}

	};
}
