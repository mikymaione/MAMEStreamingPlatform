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
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include "server_ws_impl.hpp"
#include "encoding/encode_to_movie.hpp"
#include "modules/input/input_common.h"

namespace webpp
{
	class streaming_server
	{
	private:
		bool active = false;

		unsigned long ping = 0;

		std::chrono::time_point<std::chrono::system_clock> game_start_time;

		std::chrono::time_point<std::chrono::system_clock> ping_sent =
			std::chrono::system_clock::now();

		std::unique_ptr<ws_server> server;
		std::unique_ptr<std::thread> game_thread;

		bool encoding_initialized = false;
		std::unique_ptr<encoding::encode_to_movie> encoder = nullptr;
		std::shared_ptr<ws_server::SendStream> encoder_socket = nullptr;

		event_based_device<SDL_Event>* keyboard = nullptr;

		running_machine* machine = nullptr;
		bool machine_paused_by_server = false;

	public:
		/**
		 * \brief Accept callback
		 */
		std::function<void(std::unordered_map<std::string, std::string>)> on_accept;

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

		void process_pausing_mechanism()
		{
			const auto ping_received = std::chrono::system_clock::now();
			const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(ping_received - ping_sent);

			if (milliseconds.count() > 500)
			{
				if (!machine->paused())
				{
					std::cout << "Pausing game!" << std::endl; // rimuovere
					machine_paused_by_server = true;
					//machine->pause();
				}
			}
			else
			{
				if (machine_paused_by_server && machine->paused())
				{
					std::cout << "Resuming game!" << std::endl; // rimuovere
					machine_paused_by_server = false;
					//machine->resume();
				}
			}
		}

		void send_pausing_ping()
		{
			const auto now = std::chrono::system_clock::now();
			const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - ping_sent);

			if ((machine_paused_by_server && machine->paused()) || milliseconds.count() > 2000)
			{
				ping++;
				ping_sent = std::chrono::system_clock::now();

				std::stringstream string_stream;
				string_stream << "ping:" << ping;

				send_string(string_stream.str());
			}
		}

		void send_video_size_to_client(const int w, const int h) const
		{
			std::stringstream string_stream;
			string_stream
				<< "size:"
				<< w << ":"
				<< h;

			send_string(string_stream.str());
		}

		void generate_key_event(const char* key, const std::string& down) const
		{
			SDL_Event e;
			e.type = down == "D" ? SDL_KEYDOWN : SDL_KEYUP;
			e.key.keysym.scancode = SDL_GetScancodeFromName(key);
			e.key.keysym.sym = SDL_GetKeyFromScancode(e.key.keysym.scancode);

			keyboard->queue_events(&e, 1);
		}

		void process_key(const std::vector<std::string>& values) const
		{
			const auto down = values[1];
			const auto input_number = values[2];
			const auto key = values[3];

			if (key == "PAUSE")generate_key_event("P", down);
			else if (key == "START")generate_key_event("1", down);
			else if (key == "SELECT")generate_key_event("5", down);

			else if (key == "UP")generate_key_event("Up", down);
			else if (key == "DOWN")generate_key_event("Down", down);
			else if (key == "RIGHT")generate_key_event("Right", down);
			else if (key == "LEFT")generate_key_event("Left", down);

			else if (key == "X")generate_key_event("W", down);
			else if (key == "Y")generate_key_event("E", down);
			else if (key == "A")generate_key_event("S", down);
			else if (key == "B")generate_key_event("D", down);

			else if (key == "L1")generate_key_event("Q", down);
			else if (key == "L2")generate_key_event("A", down);
			else if (key == "R1")generate_key_event("R", down);
			else if (key == "R2")generate_key_event("F", down);
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

		void set_keyboard(event_based_device<SDL_Event>* keyboard_)
		{
			keyboard = keyboard_;
		}

		void set_running_machine(running_machine* machine_)
		{
			machine = machine_;
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

#ifdef WIN32
			string_stream << "start";
#endif

			for (auto i = 0; i < argc; ++i)
			{
				string_stream << " ";
				string_stream << argv[i];
			}

#ifndef WIN32
			string_stream << " &";
#endif

			const auto command = string_stream.str();

			system(command.c_str()); //run MAME
		}

		void init_encoding(const int w, const int h, const int fps)
		{
			if (!encoding_initialized)
			{
				encoding_initialized = true;

				encoder_socket = std::make_shared<ws_server::SendStream>();

				send_video_size_to_client(w, h);

				encoder = std::make_unique<encoding::encode_to_movie>(encoder_socket, w, h, fps, [&]()
				{
					send(encoder_socket, 130);

					//send_pausing_ping();
				});
			}
		}

		/**
		* \brief Send video frame to client
		* \param pixels input pixels
		*/
		void send_video_frame(const uint8_t* pixels) const
		{
			const auto _start = std::chrono::system_clock::now();
			
			encoder->add_frame(pixels);
			
			const auto _end = std::chrono::system_clock::now();
			const auto _total = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start);

			std::cout
				<< "--Video : "
				<< _total.count()
				<< "ms"
				<< std::endl;
		}

		/**
		* \brief Add audio instant
		* \param audio_stream input audio stream
		* \param audio_stream_size input audio stream size
		* \param audio_stream_num_samples number of samples
		*/
		void send_audio_interval(const uint8_t* audio_stream, const int audio_stream_size, const int audio_stream_num_samples) const
		{
			const auto _start = std::chrono::system_clock::now();
			
			encoder->add_instant(audio_stream, audio_stream_size, audio_stream_num_samples);
			
			const auto _end = std::chrono::system_clock::now();
			const auto _total = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start);

			std::cout
				<< "--Audio : "
				<< _total.count()
				<< "ms"
				<< std::endl;
		}

		void start(const unsigned short port)
		{
			server = std::make_unique<ws_server>();
			server->config.client_mode = true;
			server->config.port = port;
			server->config.timeout_request = 0; //no timeout

			auto& endpoint = server->m_endpoint["/?"];

			endpoint.on_open = [this](auto connection)
			{
				std::cout
					<< "-Opened connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl;

				game_thread = std::make_unique<std::thread>(on_accept, connection->parameters);

				game_start_time = std::chrono::system_clock::now();

				const auto game = connection->parameters["game"];

				std::cout
					<< "Starting game: " << game
					<< std::endl;
			};

			endpoint.on_message = [this](auto connection, auto message)
			{
				const auto msg = message->string();
				const auto values = ws_server::split(msg, ":");

				if (values[0] == "ping")
					process_pausing_mechanism();
				else if (values[0] == "key")
					process_key(values);
			};

			endpoint.on_error = [](auto connection, auto code)
			{
				std::cout
					<< "-Error on connection from "
					<< connection->remote_endpoint_address
					<< ":"
					<< connection->remote_endpoint_port
					<< std::endl
					<< ": " << code.message()
					<< std::endl;
			};

			endpoint.on_close = [this](auto connection, auto status, auto reason)
			{
				const auto game_end_time = std::chrono::system_clock::now();
				const auto game_total_minute_played = std::chrono::duration_cast<std::chrono::minutes>(game_end_time - game_start_time);
				const auto game = connection->parameters["game"];

				std::cout
					<< "-Closed connection from "
					<< connection->remote_endpoint_address << ":"
					<< connection->remote_endpoint_port
					<< std::endl
					<< ": " << reason
					<< std::endl;

				std::cout
					<< game
					<< " played for: " << game_total_minute_played.count() << "min."
					<< std::endl;

				machine->schedule_exit();
			};

			std::cout
				<< "Game streaming server listening on " << port
				<< std::endl;

			server->start();

			if (game_thread->joinable())
				game_thread->join();
		}

	};
}
