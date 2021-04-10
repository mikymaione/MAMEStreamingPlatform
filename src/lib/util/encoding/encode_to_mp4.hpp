// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - (aka DinoEncoding 🦕🧡🦖)
//  Encoding to:
//  -mp4	(H.264 + AAC)
//  -webm	(VP9 + Vorbis)
//  -mpegts	(mpeg1 + mp2)
//
//============================================================
#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

// FFMPEG C headers
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

namespace encoding
{
	class encode_to_mp4
	{
	public:
		enum CODEC { MP4, WEBM, MPEGTS };

	private:
		struct StreamingContext
		{
			AVFormatContext* muxer_context = nullptr;

			AVCodec* video_codec = nullptr;
			AVCodec* audio_codec = nullptr;

			AVStream* video_stream = nullptr;
			AVStream* audio_stream = nullptr;

			AVCodecContext* video_codec_context = nullptr;
			AVCodecContext* audio_codec_context = nullptr;

			SwsContext* video_converter_context = nullptr;
			SwrContext* audio_converter_context = nullptr;
		};

	private:
		static constexpr size_t MEMORY_OUTPUT_BUFFER_SIZE = 1024 * 1024 * 200; //200MB

		CODEC codec = MPEGTS;
		std::string CONTAINER_NAME;

		// Video
		AVCodecID VIDEO_CODEC;
		static constexpr int64_t VIDEO_BIT_RATE = 64 * 1000;

		//SDL_PIXELFORMAT_RGBA32 = AV_PIX_FMT_BGR32
		//SDL_PIXELFORMAT_RGB24 = AV_PIX_FMT_RGB24
		static constexpr AVPixelFormat PIXEL_FORMAT_IN = AV_PIX_FMT_BGR32;
		static constexpr AVPixelFormat PIXEL_FORMAT_OUT = AV_PIX_FMT_YUV420P;

		// Audio
		AVCodecID AUDIO_CODEC;
		static constexpr int64_t AUDIO_BIT_RATE = 64 * 1000;

		static constexpr int AUDIO_CHANNELS_IN = 2;
		static constexpr int AUDIO_CHANNELS_OUT = 1; //1
		static constexpr uint64_t AUDIO_CHANNEL_LAYOUT_IN = AV_CH_LAYOUT_STEREO;
		static constexpr uint64_t AUDIO_CHANNEL_LAYOUT_OUT = AV_CH_LAYOUT_MONO;

		static constexpr int SAMPLE_RATE_OUT = 48000;
		static constexpr int SAMPLE_RATE_IN = 48000;
		AVSampleFormat AUDIO_SAMPLE_FORMAT_OUT;
		static constexpr AVSampleFormat AUDIO_SAMPLE_FORMAT_IN = AV_SAMPLE_FMT_S16; //AUDIO_S16SYS		

	private:
		const std::shared_ptr<std::ostream> socket;

		const int in_width, in_height;
		const int fps;

		static constexpr int out_width = 640;
		static constexpr int out_height = 480;

		const std::function<void()> on_write;

		std::chrono::time_point<std::chrono::system_clock> start_time =
			std::chrono::system_clock::now();

		char error_buffer[AV_ERROR_MAX_STRING_SIZE];

		bool header = false;
		bool sending = false;

		StreamingContext* encoder_context = nullptr;

		uint8_t* memory_output_buffer;

		AVPacket video_packet, audio_packet;

		AVFrame* wav_frame = nullptr;
		AVFrame* aac_frame = nullptr;
		uint8_t* aac_buffer = nullptr;
		uint8_t* convertedData = nullptr;

		AVFrame* rgb_frame = nullptr;
		AVFrame* yuv_frame = nullptr;
		uint8_t* yuv_buffer = nullptr;

	private:
		void error(const std::string& msg, const int error_code)
		{
			std::cerr << msg << " [" << error_code << "]" << std::endl;

			if (error_code < 0)
				if (av_strerror(error_code, error_buffer, AV_ERROR_MAX_STRING_SIZE) == 0)
					std::cerr << "--" << error_buffer << std::endl;
		}

		void die(const std::string& msg, const int error_code)
		{
			error(msg, error_code);
			exit(error_code);
		}

		static int write_packet(void* opaque, uint8_t* buf, int buf_size)
		{
			auto* const this_ = static_cast<encode_to_mp4*>(opaque);

			this_->socket->write(reinterpret_cast<const char*>(buf), buf_size);

			return 1; // 1 element wrote
		}

		void init_video()
		{
			encoder_context->video_stream = avformat_new_stream(encoder_context->muxer_context, nullptr);
			encoder_context->video_stream->id = 0;

			encoder_context->video_codec = avcodec_find_encoder(VIDEO_CODEC);
			if (!encoder_context->video_codec)
				die("could not find the proper codec", 0);

			encoder_context->video_codec_context = avcodec_alloc_context3(encoder_context->video_codec);
			if (!encoder_context->video_codec_context)
				die("Could not allocated memory for codec context", 0);

			if (VIDEO_CODEC != AV_CODEC_ID_MPEG1VIDEO)
			{
				std::cout << "AV_CODEC_FLAG_GLOBAL_HEADER" << std::endl;
				encoder_context->video_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				encoder_context->video_codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
			}

			encoder_context->video_codec_context->width = out_width;
			encoder_context->video_codec_context->height = out_height;

			encoder_context->video_codec_context->bit_rate = VIDEO_BIT_RATE;
			encoder_context->video_codec_context->bit_rate_tolerance = 0;

			encoder_context->video_codec_context->max_b_frames = 0;
			encoder_context->video_codec_context->has_b_frames = 0;
			encoder_context->video_codec_context->delay = 0;
			//encoder_context->video_codec_context->gop_size = 10;

			encoder_context->video_codec_context->pix_fmt = PIXEL_FORMAT_OUT;

			encoder_context->video_codec_context->framerate = { fps, 1 };
			encoder_context->video_codec_context->time_base = { 1, fps };
			encoder_context->video_stream->time_base = encoder_context->video_codec_context->time_base;

			AVDictionary* options = nullptr;
			if (VIDEO_CODEC == AV_CODEC_ID_VP8)
			{
				//
			}
			else if (VIDEO_CODEC == AV_CODEC_ID_VP9)
			{
				av_dict_set(&options, "deadline", "realtime", 0);
			}
			else if (VIDEO_CODEC == AV_CODEC_ID_H264)
			{
				av_dict_set(&options, "preset", "ultrafast", 0);
				av_dict_set(&options, "tune", "zerolatency", 0);
			}

			auto ret = avcodec_open2(encoder_context->video_codec_context, encoder_context->video_codec, &options);
			if (ret < 0)
				die("Could not open the codec", ret);

			av_dict_free(&options);

			ret = avcodec_parameters_from_context(encoder_context->video_stream->codecpar, encoder_context->video_codec_context);
			if (ret < 0)
				die("Could not retrieve parameters from context", ret);

			encoder_context->video_converter_context = sws_getContext(
				in_width, in_height, PIXEL_FORMAT_IN,
				out_width, out_height, PIXEL_FORMAT_OUT,
				SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
			);

			rgb_frame = av_frame_alloc();
			rgb_frame->width = in_width;
			rgb_frame->height = in_height;
			rgb_frame->format = PIXEL_FORMAT_IN;

			yuv_frame = av_frame_alloc();
			yuv_frame->width = out_width;
			yuv_frame->height = out_height;
			yuv_frame->format = PIXEL_FORMAT_OUT;

			const auto yuv_buffer_size = out_width * out_height * 3 / 2;
			yuv_buffer = new uint8_t[yuv_buffer_size];

			avpicture_fill(
				reinterpret_cast<AVPicture*>(yuv_frame), yuv_buffer,
				PIXEL_FORMAT_OUT,
				out_width, out_height);
		}

		void init_audio()
		{
			encoder_context->audio_stream = avformat_new_stream(encoder_context->muxer_context, nullptr);
			encoder_context->audio_stream->id = 1;

			encoder_context->audio_codec = avcodec_find_encoder(AUDIO_CODEC);
			if (!encoder_context->audio_codec)
				die("Could not find the proper codec", 0);

			encoder_context->audio_codec_context = avcodec_alloc_context3(encoder_context->audio_codec);
			if (!encoder_context->audio_codec_context)
				die("Could not allocated memory for codec context", 0);

			encoder_context->audio_codec_context->channels = AUDIO_CHANNELS_OUT;
			encoder_context->audio_codec_context->channel_layout = av_get_default_channel_layout(encoder_context->audio_codec_context->channels);
			encoder_context->audio_codec_context->sample_rate = SAMPLE_RATE_OUT;
			encoder_context->audio_codec_context->sample_fmt = AUDIO_SAMPLE_FORMAT_OUT;

			encoder_context->audio_codec_context->bit_rate = AUDIO_BIT_RATE;

			encoder_context->audio_codec_context->time_base = { 1,fps };
			//encoder_context->audio_codec_context->time_base = { 1,SAMPLE_RATE_OUT };
			//encoder_context->audio_stream->time_base = encoder_context->audio_codec_context->time_base;

			AVDictionary* options = nullptr;

			auto ret = avcodec_open2(encoder_context->audio_codec_context, encoder_context->audio_codec, &options);
			if (ret < 0)
				die("Could not open the codec", ret);

			av_dict_free(&options);

			avcodec_parameters_from_context(encoder_context->audio_stream->codecpar, encoder_context->audio_codec_context);

			aac_frame = av_frame_alloc();
			aac_frame->nb_samples = encoder_context->audio_codec_context->frame_size; //1024;
			aac_frame->channels = encoder_context->audio_codec_context->channels;
			aac_frame->channel_layout = encoder_context->audio_codec_context->channel_layout;
			aac_frame->format = encoder_context->audio_codec_context->sample_fmt;
			aac_frame->sample_rate = encoder_context->audio_codec_context->sample_rate;

			wav_frame = av_frame_alloc();
			wav_frame->channels = AUDIO_CHANNELS_IN;
			wav_frame->channel_layout = av_get_default_channel_layout(AUDIO_CHANNELS_IN);
			wav_frame->format = AUDIO_SAMPLE_FORMAT_IN;
			wav_frame->sample_rate = SAMPLE_RATE_IN;

			// Resampler
			encoder_context->audio_converter_context = swr_alloc_set_opts(
				nullptr,
				AUDIO_CHANNEL_LAYOUT_OUT,
				AUDIO_SAMPLE_FORMAT_OUT,
				SAMPLE_RATE_OUT,
				AUDIO_CHANNEL_LAYOUT_IN,
				AUDIO_SAMPLE_FORMAT_IN,
				SAMPLE_RATE_IN,
				0,
				nullptr
			);

			swr_init(encoder_context->audio_converter_context);

			ret = av_samples_alloc(
				&aac_buffer,
				nullptr,
				encoder_context->audio_codec_context->channels,
				aac_frame->nb_samples,
				encoder_context->audio_codec_context->sample_fmt,
				1);
			if (ret < 0)
				die("Cannot alloc audio conversion buffer", ret);
		}

		void set_container_options(AVDictionary* options) const
		{
			switch (codec)
			{
				case MP4:
					//av_dict_set(&options, "movflags", "empty_moov+frag_keyframe+default_base_moof+faststart", 0); //xpra
					//av_dict_set(&options, "movflags", "empty_moov+omit_tfhd_offset+separate_moof+frag_custom", 0);
					av_dict_set(&options, "movflags", "+frag_keyframe+omit_tfhd_offset+empty_moov+default_base_moof", 0);
					av_dict_set(&options, "frag_duration", "700000", 0);
					break;
				case WEBM:
					av_dict_set(&options, "movflags", "dash+live", 0);
					break;
				case MPEGTS:
					break;
			}
		}

		void write_header()
		{
			header = true;

			AVDictionary* options = nullptr;
			set_container_options(options);

			const auto ret = avformat_write_header(encoder_context->muxer_context, &options);
			if (ret < 0)
				die("Could not write header", ret);

			av_dict_free(&options);
		}

		void send_it()
		{
			if (!sending)
			{
				sending = true;

				const auto end_time = std::chrono::system_clock::now();
				const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

				if (milliseconds.count() > 70)
				{
					const auto ret = av_write_trailer(encoder_context->muxer_context);
					if (ret < 0)
						die("Error writing trailer", ret);

					on_write();

					start_time = std::chrono::system_clock::now();
					header = false;
				}

				sending = false;
			}
		}

	public:
		encode_to_mp4(const std::shared_ptr<std::ostream>& socket, const int in_width, const int in_height, const int fps, std::function<void()> on_write) :
			socket(socket),
			in_width(in_width),
			in_height(in_height),
			fps(fps),
			on_write(on_write)
		{
			//av_log_set_level(AV_LOG_DEBUG);

			av_register_all();
			avcodec_register_all();

			switch (codec)
			{
				case MP4:
					CONTAINER_NAME = "mp4";
					VIDEO_CODEC = AV_CODEC_ID_H264;
					AUDIO_CODEC = AV_CODEC_ID_AAC;
					AUDIO_SAMPLE_FORMAT_OUT = AV_SAMPLE_FMT_FLTP;
					break;
				case WEBM:
					CONTAINER_NAME = "webm";
					VIDEO_CODEC = AV_CODEC_ID_VP9;
					AUDIO_CODEC = AV_CODEC_ID_VORBIS;
					AUDIO_SAMPLE_FORMAT_OUT = AV_SAMPLE_FMT_FLTP;
					break;
				case MPEGTS:
					CONTAINER_NAME = "mpegts";
					VIDEO_CODEC = AV_CODEC_ID_MPEG1VIDEO;
					AUDIO_CODEC = AV_CODEC_ID_MP2;
					AUDIO_SAMPLE_FORMAT_OUT = AV_SAMPLE_FMT_S16;
					break;
			}

			encoder_context = new StreamingContext();

			auto ret = avformat_alloc_output_context2(&encoder_context->muxer_context, nullptr, CONTAINER_NAME.c_str(), nullptr);
			if (ret < 0)
				die("Cannot allocate output context", ret);

			memory_output_buffer = static_cast<uint8_t*>(av_malloc(MEMORY_OUTPUT_BUFFER_SIZE));

			encoder_context->muxer_context->flags = AVFMT_FLAG_CUSTOM_IO;
			encoder_context->muxer_context->pb = avio_alloc_context(memory_output_buffer, MEMORY_OUTPUT_BUFFER_SIZE, 1, this, nullptr, write_packet, nullptr);
			if (encoder_context->muxer_context->pb == nullptr)
				die("Cannot allocate context", ret);

			encoder_context->muxer_context->flush_packets = 1;
			encoder_context->muxer_context->start_time = 0;
			encoder_context->muxer_context->strict_std_compliance = 1;

			init_video();
			init_audio();
		}

		~encode_to_mp4()
		{
			avformat_close_input(&encoder_context->muxer_context);
			avformat_free_context(encoder_context->muxer_context);

			//Video
			sws_freeContext(encoder_context->video_converter_context);

			avcodec_close(encoder_context->video_codec_context);
			av_free(encoder_context->video_codec_context);

			av_frame_unref(rgb_frame);
			av_frame_unref(yuv_frame);

			delete[] yuv_buffer;

			//Audio
			swr_free(&encoder_context->audio_converter_context);

			avcodec_close(encoder_context->audio_codec_context);
			av_free(encoder_context->audio_codec_context);

			av_frame_unref(aac_frame);

			av_free(memory_output_buffer);

			av_freep(&aac_buffer);

			delete encoder_context;
		}

		/**
		 * \brief Add video frame
		 * \param pixels input pixels
		 */
		void add_frame(const uint8_t* pixels)
		{
			if (!header)
				write_header();

			avpicture_fill(
				reinterpret_cast<AVPicture*>(rgb_frame),
				pixels,
				PIXEL_FORMAT_IN,
				in_width, in_height);

			//RGB to YUV
			sws_scale(
				encoder_context->video_converter_context,
				rgb_frame->data, rgb_frame->linesize, 0, in_height,
				yuv_frame->data, yuv_frame->linesize
			);

			av_init_packet(&video_packet);
			video_packet.data = nullptr;
			video_packet.size = 0;

			avcodec_send_frame(encoder_context->video_codec_context, yuv_frame);
			const auto got_packet_ptr = avcodec_receive_packet(encoder_context->video_codec_context, &video_packet) == 0;

			if (got_packet_ptr)
			{
				const auto ret = av_interleaved_write_frame(encoder_context->muxer_context, &video_packet);

				if (ret == 0)
					send_it();
				else
					error("Error while writing video frame", ret);
			}

			av_packet_unref(&video_packet);
		}

		/**
		 * \brief Add audio instant
		 * \param audio_stream input audio stream
		 * \param audio_stream_size input audio stream size
		 * \param audio_stream_num_samples number of samples
		 */
		void add_instant(const uint8_t* audio_stream, const int audio_stream_size, const int audio_stream_num_samples)
		{
			if (!header)
				write_header();

			wav_frame->nb_samples = audio_stream_num_samples;

			auto ret = avcodec_fill_audio_frame(
				wav_frame,
				AUDIO_CHANNELS_IN,
				AUDIO_SAMPLE_FORMAT_IN,
				audio_stream,
				audio_stream_size,
				1); //no-alignment
			if (ret < 0)
				die("Cannot fill audio frame", ret);

			if (convertedData == nullptr)
			{
				ret = av_samples_alloc(
					&convertedData,
					nullptr,
					AUDIO_CHANNELS_OUT,
					aac_frame->nb_samples,
					AUDIO_SAMPLE_FORMAT_OUT,
					0);
				if (ret < 0)
					die("Could not allocate samples", ret);
			}

			auto outSamples = swr_convert(
				encoder_context->audio_converter_context,
				// output
				nullptr, 0,
				// input
				const_cast<const uint8_t**>(wav_frame->data), wav_frame->nb_samples);
			if (outSamples < 0)
				die("Could not convert", outSamples);

			for (;;)
			{
				outSamples = swr_get_out_samples(encoder_context->audio_converter_context, 0);
				if (outSamples < encoder_context->audio_codec_context->frame_size * encoder_context->audio_codec_context->channels)
					break;

				swr_convert(
					encoder_context->audio_converter_context,
					// output
					&convertedData, aac_frame->nb_samples,
					// input
					nullptr, 0);

				const auto buffer_size = av_samples_get_buffer_size(
					nullptr,
					encoder_context->audio_codec_context->channels,
					aac_frame->nb_samples,
					encoder_context->audio_codec_context->sample_fmt,
					0);
				if (buffer_size < 0)
					die("Invalid buffer size", buffer_size);

				ret = avcodec_fill_audio_frame(
					aac_frame,
					encoder_context->audio_codec_context->channels,
					encoder_context->audio_codec_context->sample_fmt,
					convertedData,
					buffer_size,
					0);
				if (ret < 0)
					die("Could not fill frame", ret);

				av_init_packet(&audio_packet);
				audio_packet.data = nullptr;
				audio_packet.size = 0;

				int got_packet_ptr;
				ret = avcodec_encode_audio2(encoder_context->audio_codec_context, &audio_packet, aac_frame, &got_packet_ptr);
				if (ret < 0)
					die("Error encoding audio frame", ret);

				if (got_packet_ptr)
				{
					audio_packet.stream_index = 1;

					ret = av_interleaved_write_frame(encoder_context->muxer_context, &audio_packet);

					if (ret == 0)
						send_it();
					else
						error("Error while writing audio frame", ret);
				}

				av_packet_unref(&audio_packet);
			}
		}

	};
}
