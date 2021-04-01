// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - Encoding to MP4 (H.264 + AAC) (aka DinoEncoding 🦕🧡🦖)
//
//============================================================
#pragma once

#include <chrono>
#include <ostream>
#include <queue>

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
		};

		struct Encoder_pts
		{
			int64_t video_pts, frame_duration;
		};

	private:
		static constexpr size_t MEMORY_OUTPUT_BUFFER_SIZE = 1024 * 1024 * 100; //100MB

		const char* CONTAINER_NAME = "rawvideo";

		// Video
		static constexpr AVCodecID VIDEO_CODEC = AV_CODEC_ID_H264;
		static constexpr int64_t VIDEO_BIT_RATE = 128 * 1024;

		//SDL_PIXELFORMAT_RGBA32 = AV_PIX_FMT_BGR32
		//SDL_PIXELFORMAT_RGB24 = AV_PIX_FMT_RGB24
		static constexpr AVPixelFormat PIXEL_FORMAT_IN = AV_PIX_FMT_BGR32;
		static constexpr AVPixelFormat PIXEL_FORMAT_OUT = AV_PIX_FMT_YUV420P;

		// Audio
		static constexpr AVCodecID AUDIO_CODEC = AV_CODEC_ID_AAC;
		static constexpr int64_t AUDIO_BIT_RATE = 96 * 1024;

		static constexpr int AUDIO_CHANNELS_IN = 2;
		static constexpr int AUDIO_CHANNELS_OUT = 2; //1
		static constexpr uint64_t AUDIO_CHANNEL_LAYOUT_IN = AV_CH_LAYOUT_STEREO;
		static constexpr uint64_t AUDIO_CHANNEL_LAYOUT_OUT = AV_CH_LAYOUT_STEREO; //AV_CH_LAYOUT_MONO

		static constexpr int SAMPLE_RATE_OUT = 48000; //44100;
		static constexpr int SAMPLE_RATE_IN = 48000;
		static constexpr AVSampleFormat AUDIO_SAMPLE_FORMAT_OUT = AV_SAMPLE_FMT_FLTP;
		static constexpr AVSampleFormat AUDIO_SAMPLE_FORMAT_IN = AV_SAMPLE_FMT_S16; //AUDIO_S16SYS

	public:
		std::function<int(uint8_t* buf, int buf_size)> on_write;

	private:
		int in_width, in_height, out_width, out_height;
		const int fps;

	private:
		std::chrono::time_point<std::chrono::system_clock> start_time =
			std::chrono::system_clock::now();

		char error_buffer[AV_ERROR_MAX_STRING_SIZE];

		StreamingContext* encoder_context = nullptr;
		Encoder_pts* video_encoder_pts = nullptr;

		uint8_t* memory_output_buffer;

		bool got_packet_ptr = false;
		AVPacket video_packet, audio_packet;

		AVFrame* aac_frame = nullptr;
		uint8_t* aac_buffer = nullptr;

		uint8_t* audio_stream_temp = nullptr;
		uint8_t audio_stream_temp_shift = 0;

		AVFrame* rgb_frame = nullptr;
		AVFrame* yuv_frame = nullptr;
		uint8_t* yuv_buffer = nullptr;

	private:
		void die(const std::string& msg, const int error_code)
		{
			std::cerr << msg << " [" << error_code << "]" << std::endl;

			if (error_code < 0)
				if (av_strerror(error_code, error_buffer, AV_ERROR_MAX_STRING_SIZE) == 0)
					std::cerr << "--" << error_buffer << std::endl;

			exit(error_code);
		}

		static int write_buffer(void* opaque, uint8_t* buf, int buf_size)
		{
			auto* const this_ = static_cast<encode_to_mp4*>(opaque);

			return this_->on_write(buf, buf_size);
		}

		void init_video()
		{
			encoder_context->video_stream = avformat_new_stream(encoder_context->format_context, nullptr);

			encoder_context->video_codec = avcodec_find_encoder(VIDEO_CODEC);
			if (!encoder_context->video_codec)
				die("could not find the proper codec", 0);

			encoder_context->video_codec_context = avcodec_alloc_context3(encoder_context->video_codec);
			if (!encoder_context->video_codec_context)
				die("Could not allocated memory for codec context", 0);

			encoder_context->video_codec_context->width = out_width;
			encoder_context->video_codec_context->height = out_height;

			encoder_context->video_codec_context->bit_rate = VIDEO_BIT_RATE;

			encoder_context->video_codec_context->pix_fmt = PIXEL_FORMAT_OUT;
			encoder_context->video_codec_context->framerate = { fps ,1 };
			encoder_context->video_codec_context->time_base = { 1,fps };
			encoder_context->video_stream->time_base = encoder_context->video_codec_context->time_base;

			AVDictionary* options = nullptr;
			av_dict_set(&options, "preset", "ultrafast", 0);
			av_dict_set(&options, "tune", "zerolatency", 0);
			//av_dict_set(&options, "crf", "40", 0);

			auto ret = avcodec_open2(encoder_context->video_codec_context, encoder_context->video_codec, &options);
			if (ret < 0)
				die("Could not open the codec", ret);

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
			encoder_context->audio_stream = avformat_new_stream(encoder_context->format_context, nullptr);

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

			encoder_context->audio_codec_context->time_base = { 1,SAMPLE_RATE_OUT };
			encoder_context->audio_stream->time_base = encoder_context->audio_codec_context->time_base;

			AVDictionary* options = nullptr;
			av_dict_set(&options, "movflags", "+faststart", 0);

			auto ret = avcodec_open2(encoder_context->audio_codec_context, encoder_context->audio_codec, &options);
			if (ret < 0)
				die("Could not open the codec", ret);

			avcodec_parameters_from_context(encoder_context->audio_stream->codecpar, encoder_context->audio_codec_context);

			aac_frame = av_frame_alloc();
			aac_frame->nb_samples = encoder_context->audio_codec_context->frame_size; //1024;
			aac_frame->channels = encoder_context->audio_codec_context->channels;
			aac_frame->channel_layout = encoder_context->audio_codec_context->channel_layout;
			aac_frame->format = encoder_context->audio_codec_context->sample_fmt;
			aac_frame->sample_rate = encoder_context->audio_codec_context->sample_rate;

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

		void send_it()
		{
			const auto ret = av_write_trailer(encoder_context->format_context);
			if (ret < 0)
				die("Error writing trailer", ret);

			write_header();
		}

		void write_header()
		{
			AVDictionary* options = nullptr;

			const auto ret = avformat_write_header(encoder_context->format_context, &options);
			if (ret < 0)
				die("Could not write header", ret);
		}

		void write_video_pts()
		{
			video_packet.pts = video_encoder_pts->video_pts;
			video_encoder_pts->video_pts += video_encoder_pts->frame_duration;
			video_packet.dts = video_packet.pts;
			video_packet.duration = video_encoder_pts->frame_duration;
			video_packet.stream_index = 0;
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

			video_encoder_pts = new Encoder_pts();
			encoder_context = new StreamingContext();

			memory_output_buffer = static_cast<uint8_t*>(av_malloc(MEMORY_OUTPUT_BUFFER_SIZE));
			encoder_context->io_context = avio_alloc_context(memory_output_buffer, MEMORY_OUTPUT_BUFFER_SIZE, 1, this, nullptr, write_buffer, nullptr);

			avformat_alloc_output_context2(&encoder_context->format_context, nullptr, CONTAINER_NAME, nullptr);

			encoder_context->format_context->pb = encoder_context->io_context;
			encoder_context->format_context->flags = AVFMT_FLAG_CUSTOM_IO;

			//if (encoder_context->format_context->oformat->flags & AVFMT_GLOBALHEADER)
				//encoder_context->format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			init_video();
			init_audio();

			write_header();
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

			av_free(memory_output_buffer);

			av_freep(&aac_buffer);

			delete[] audio_stream_temp;

			delete encoder_context;
			delete video_encoder_pts;
		}

		/**
		 * \brief Add video frame
		 * \param pixels
		 */
		void add_frame(const uint8_t* pixels)
		{
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

			avcodec_send_frame(encoder_context->video_codec_context, yuv_frame);
			got_packet_ptr = avcodec_receive_packet(encoder_context->video_codec_context, &video_packet) == 0;

			if (got_packet_ptr)
			{
				write_video_pts();

				const auto ret = av_interleaved_write_frame(encoder_context->format_context, &video_packet);
				if (ret < 0)
					die("Error while writing video frame", ret);

				const auto end_time = std::chrono::system_clock::now();
				const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

				if (milliseconds.count() > 100)
				{
					send_it();
					start_time = std::chrono::system_clock::now();
				}
			}
		}

		/**
		 * \brief Add audio instant
		 * \param audio_stream
		 * \param audio_stream_size
		 * \param audio_stream_num_samples
		 */
		void add_instant(const uint8_t* audio_stream, const int audio_stream_size, const int audio_stream_num_samples)
		{
			if (audio_stream_temp == nullptr)
				audio_stream_temp = new uint8_t[audio_stream_size * 2];

			for (auto i = 0; i < audio_stream_size; i++)
				audio_stream_temp[i + audio_stream_temp_shift] = audio_stream[i];

			audio_stream_temp_shift += audio_stream_size;

			if (audio_stream_temp_shift == audio_stream_size * 2)
			{
				audio_stream_temp_shift = 0;

				auto ret = swr_convert(
					encoder_context->audio_converter_context,
					// output
					&aac_buffer, aac_frame->nb_samples,
					// input
					const_cast<const uint8_t**>(&audio_stream_temp), aac_frame->nb_samples);
				if (ret < 0)
					die("Cannot convert audio", ret);

				const auto aac_buffer_size = av_samples_get_buffer_size(
					nullptr,
					encoder_context->audio_codec_context->channels,
					aac_frame->nb_samples,
					encoder_context->audio_codec_context->sample_fmt,
					1);

				ret = avcodec_fill_audio_frame(
					aac_frame,
					encoder_context->audio_codec_context->channels,
					encoder_context->audio_codec_context->sample_fmt,
					aac_buffer,
					aac_buffer_size,
					1); //no-alignment
				if (ret < 0)
					die("Cannot fill audio frame", ret);

				av_init_packet(&audio_packet);

				ret = avcodec_send_frame(encoder_context->audio_codec_context, aac_frame);
				if (ret < 0)
					die("Cannot encode audio frame", ret);

				got_packet_ptr = avcodec_receive_packet(encoder_context->audio_codec_context, &audio_packet) == 0;

				if (got_packet_ptr)
				{
					audio_packet.stream_index = 1;

					ret = av_interleaved_write_frame(encoder_context->format_context, &audio_packet);
					if (ret < 0)
						die("Error while writing audio frame", ret);
				}

				av_packet_unref(&audio_packet);
			}
		}

	};
}
