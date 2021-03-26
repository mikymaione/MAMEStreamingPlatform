// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - Encoding to MP4 (H.264 + AAC) (aka DinoEncoding 🦕🧡🦖)
//
//============================================================
#pragma once

#include <ostream>

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
	private:
		struct StreamingContext
		{
			AVIOContext* io_context = nullptr;
			AVFormatContext* format_context = nullptr;

			AVCodec* video_codec = nullptr;
			AVCodec* audio_codec = nullptr;

			AVStream* video_stream = nullptr;
			AVStream* audio_stream = nullptr;

			AVCodecContext* video_codec_context = nullptr;
			AVCodecContext* audio_codec_context = nullptr;

			SwsContext* video_converter_context = nullptr;
			SwrContext* audio_converter_context = nullptr;

			int video_index = 0;
			int audio_index = 1;
		};

	private:
		static const int memory_kb = 1024 * 5;

		// Video
		static const AVCodecID video_codec = AV_CODEC_ID_MPEG1VIDEO;

		//SDL_PIXELFORMAT_RGBA32 = AV_PIX_FMT_BGR32
		//SDL_PIXELFORMAT_RGB24 = AV_PIX_FMT_RGB24
		static const AVPixelFormat Pixel_Format_in = AV_PIX_FMT_BGR32;
		static const AVPixelFormat Pixel_Format_out = AV_PIX_FMT_YUV420P;

		// Audio
		static const AVCodecID audio_codec = AV_CODEC_ID_MP2;

		static const int audio_channels_in = 2;
		static const int audio_channels_out = 1;
		static const uint64_t audio_channel_layout_in = AV_CH_LAYOUT_STEREO;
		static const uint64_t audio_channel_layout_out = AV_CH_LAYOUT_MONO;

		static const int out_sample_rate = 48000; //44100;
		static const int in_sample_rate = 48000;
		static const int samples = 512;
		static const AVSampleFormat audio_sample_format_out = AV_SAMPLE_FMT_S16;
		static const AVSampleFormat audio_sample_format_in = AV_SAMPLE_FMT_S16;

	public:
		std::function<int(uint8_t* buf, int buf_size)> on_write;

	private:
		int in_width, in_height, out_width, out_height;
		const int fps;

	private:
		StreamingContext* encoder_context = nullptr;

		AVPacket video_packet, audio_packet;

		AVFrame* aac_frame = nullptr;

		AVFrame* rgb_frame = nullptr;
		AVFrame* yuv_frame = nullptr;

		uint8_t* yuv_buffer = nullptr;

		uint8_t* aac_buffer[audio_channels_out];
		const uint8_t* wav_buffer[audio_channels_out];

		uint8_t* memory_output_buffer;

		unsigned aframe = 0;
		unsigned vframe = 0;

		bool got_packet_ptr = false;

	private:
		static void die(const std::string& msg)
		{
			std::cerr << msg << std::endl;
			exit(-100);
		}

		static int write_buffer(void* opaque, uint8_t* buf, int buf_size)
		{
			const auto this_ = static_cast<encode_to_mp4*>(opaque);
			std::cout << "Sending " << buf_size << "bits" << std::endl;

			return this_->on_write(buf, buf_size);
		}

		void init_video()
		{
			encoder_context->video_stream = avformat_new_stream(encoder_context->format_context, nullptr);

			encoder_context->video_codec = avcodec_find_encoder(video_codec);
			if (!encoder_context->video_codec)
				die("could not find the proper codec");

			encoder_context->video_codec_context = avcodec_alloc_context3(encoder_context->video_codec);
			if (!encoder_context->video_codec_context)
				die("could not allocated memory for codec context");

			encoder_context->video_codec_context->width = out_width;
			encoder_context->video_codec_context->height = out_height;

			encoder_context->video_codec_context->pix_fmt = Pixel_Format_out;
			encoder_context->video_codec_context->time_base = { 1,fps };
			encoder_context->video_stream->time_base = encoder_context->video_codec_context->time_base;

			if (avcodec_open2(encoder_context->video_codec_context, encoder_context->video_codec, nullptr) < 0)
				die("could not open the codec");

			avcodec_parameters_from_context(encoder_context->video_stream->codecpar, encoder_context->video_codec_context);

			encoder_context->video_converter_context = sws_getContext(
				in_width, in_height, Pixel_Format_in,
				out_width, out_height, Pixel_Format_out,
				SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
			);

			rgb_frame = av_frame_alloc();
			rgb_frame->width = in_width;
			rgb_frame->height = in_height;
			rgb_frame->format = Pixel_Format_in;

			yuv_frame = av_frame_alloc();
			yuv_frame->width = out_width;
			yuv_frame->height = out_height;
			yuv_frame->format = Pixel_Format_out;

			const auto yuv_buffer_size = out_width * out_height * 3 / 2;
			yuv_buffer = new uint8_t[yuv_buffer_size];

			avpicture_fill(
				reinterpret_cast<AVPicture*>(yuv_frame), yuv_buffer,
				Pixel_Format_out,
				out_width, out_height);
		}

		void init_audio()
		{
			encoder_context->audio_stream = avformat_new_stream(encoder_context->format_context, nullptr);

			encoder_context->audio_codec = avcodec_find_encoder(audio_codec);
			if (!encoder_context->audio_codec)
				die("could not find the proper codec");

			encoder_context->audio_codec_context = avcodec_alloc_context3(encoder_context->audio_codec);
			if (!encoder_context->audio_codec_context)
				die("could not allocated memory for codec context");

			encoder_context->audio_codec_context->channels = 1;
			encoder_context->audio_codec_context->channel_layout = av_get_default_channel_layout(encoder_context->audio_codec_context->channels);

			encoder_context->audio_codec_context->sample_rate = out_sample_rate;
			encoder_context->audio_codec_context->sample_fmt = audio_sample_format_out;

			encoder_context->audio_codec_context->time_base = { 1,out_sample_rate };
			encoder_context->audio_stream->time_base = encoder_context->audio_codec_context->time_base;

			if (avcodec_open2(encoder_context->audio_codec_context, encoder_context->audio_codec, nullptr) < 0)
				die("could not open the codec");

			avcodec_parameters_from_context(encoder_context->audio_stream->codecpar, encoder_context->audio_codec_context);

			aac_frame = av_frame_alloc();
			aac_frame->nb_samples = samples;
			aac_frame->channels = encoder_context->audio_codec_context->channels;
			aac_frame->channel_layout = encoder_context->audio_codec_context->channel_layout;
			aac_frame->format = encoder_context->audio_codec_context->sample_fmt;
			aac_frame->sample_rate = encoder_context->audio_codec_context->sample_rate;

			// Resampler
			encoder_context->audio_converter_context = swr_alloc_set_opts(
				nullptr,
				audio_channel_layout_out,
				audio_sample_format_out,
				out_sample_rate,
				audio_channel_layout_in,
				audio_sample_format_in,
				in_sample_rate,
				0,
				nullptr
			);

			swr_init(encoder_context->audio_converter_context);
		}

		void send_it()
		{
			if (vframe > 5 || aframe > 5)
			{
				aframe = 0;
				vframe = 0;

				//if (av_write_trailer(encoder_context->format_context) != 0)
					//die("Error writing trailer");
			}
		}

	public:
		encode_to_mp4(const int in_width, const int in_height,
					  const int out_width, const int out_height,
					  const int fps) :
			in_width(in_width),
			in_height(in_height),
			out_width(out_width),
			out_height(out_height),
			fps(fps)
		{
			av_register_all();
			avcodec_register_all();

			encoder_context = new StreamingContext();

			memory_output_buffer = static_cast<uint8_t*>(av_malloc(memory_kb));
			encoder_context->io_context = avio_alloc_context(memory_output_buffer, memory_kb, 1, this, nullptr, write_buffer, nullptr);

			avformat_alloc_output_context2(&encoder_context->format_context, nullptr, "mpegts", nullptr);

			encoder_context->format_context->pb = encoder_context->io_context;
			encoder_context->format_context->flags = AVFMT_FLAG_CUSTOM_IO;

			init_video();
			init_audio();

			if (encoder_context->format_context->oformat->flags & AVFMT_GLOBALHEADER)
				encoder_context->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			AVDictionary* options = nullptr;

			if (avformat_write_header(encoder_context->format_context, &options) < 0)
				die("an error occurred when opening output file");
		}

		~encode_to_mp4()
		{
			avformat_close_input(&encoder_context->format_context);
			avformat_free_context(encoder_context->format_context);

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

			delete encoder_context;
		}

		/**
		 * \brief Change input size
		 * \param new_in_width
		 * \param new_in_height
		 */
		void set_input_size(const int new_in_width, const int new_in_height)
		{
			in_width = new_in_width;
			in_height = new_in_height;
		}

		/**
		 * \brief Change output size
		 * \param new_out_width
		 * \param new_out_height
		 */
		void set_output_size(const int new_out_width, const int new_out_height)
		{
			out_width = new_out_width;
			out_height = new_out_height;
		}

		/**
		 * \brief Add audio instant
		 * \param audio_stream
		 * \param audio_stream_size
		 * \param ws_stream
		 * \return success
		 */
		void add_instant(const uint8_t* audio_stream, const int audio_stream_size)
		{
			const auto temp = new uint8_t[audio_stream_size];
			aac_buffer[0] = temp;
			wav_buffer[0] = audio_stream;

			swr_convert(
				encoder_context->audio_converter_context,
				aac_buffer, audio_stream_size, // destination
				wav_buffer, audio_stream_size); //source

			avcodec_fill_audio_frame(
				aac_frame,
				encoder_context->audio_codec_context->channels,
				encoder_context->audio_codec_context->sample_fmt,
				temp,
				audio_stream_size,
				1 /*no-alignment*/);

			av_init_packet(&audio_packet);

			avcodec_send_frame(encoder_context->audio_codec_context, aac_frame);
			got_packet_ptr = avcodec_receive_packet(encoder_context->audio_codec_context, &audio_packet) == 0;

			audio_packet.stream_index = 1;

			if (got_packet_ptr)
			{
				if (av_interleaved_write_frame(encoder_context->format_context, &audio_packet) != 0)
					die("Error while writing audio frame");

				aframe++;
			}

			av_packet_unref(&audio_packet);

			delete[] temp;

			send_it();
		}

		/**
		 * \brief Add video frame
		 * \param pixels
		 * \param ws_stream
		 * \return success
		 */
		void add_frame(const uint8_t* pixels)
		{
			avpicture_fill(
				reinterpret_cast<AVPicture*>(rgb_frame),
				pixels,
				Pixel_Format_in,
				in_width, in_height);

			//RGB to YUV
			sws_scale(
				encoder_context->video_converter_context,
				rgb_frame->data, rgb_frame->linesize, 0, in_height,
				yuv_frame->data, yuv_frame->linesize
			);

			av_init_packet(&video_packet);

			avcodec_send_frame(encoder_context->video_codec_context, yuv_frame);
			got_packet_ptr = avcodec_receive_packet(encoder_context->video_codec_context, &video_packet) == 0;

			video_packet.stream_index = 0;

			if (got_packet_ptr)
			{
				if (av_interleaved_write_frame(encoder_context->format_context, &video_packet) != 0)
					die("Error while writing video frame");

				vframe++;
			}

			send_it();
		}

	};
}
