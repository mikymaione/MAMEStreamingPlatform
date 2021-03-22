// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - Encoding to MP4 (H.264 + AAC) (aka DinoEncoding 🦕🧡🦖)
//
//============================================================
#pragma once

#include <memory>
#include <ostream>

// FFMPEG C headers
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
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
		// Video
		static const AVCodecID video_codec = AV_CODEC_ID_H264;

		//SDL_PIXELFORMAT_RGBA32 = AV_PIX_FMT_BGR32
		//SDL_PIXELFORMAT_RGB24 = AV_PIX_FMT_RGB24
		static const AVPixelFormat SDL_pixel_format = AV_PIX_FMT_BGR32;
		static const AVPixelFormat H264_pixel_format = AV_PIX_FMT_YUV420P;

		int in_width, in_height, out_width, out_height;
		const int channels, fps;

		AVPacket video_packet, audio_packet;
		AVFrame* sound_in_frame = nullptr;
		AVFrame* rgb_frame = nullptr;
		AVFrame* yuv_frame = nullptr;
		AVDictionary* video_options = nullptr;
		AVDictionary* audio_options = nullptr;
		AVCodecContext* video_codec_context = nullptr;
		SwsContext* video_converter_context = nullptr;
		uint8_t* yuv_buffer = nullptr;
		uint8_t* video_packet_buffer = nullptr;

		int video_packet_buffer_size, got_packet_ptr;
		// Video

		// Audio
		static const AVCodecID audio_codec = AV_CODEC_ID_AAC;

		static const int SWR_CH_MAX = 32;
		static const int out_sample_rate = 48000; //44100;
		static const int in_sample_rate = 48000;
		static const int samples = 512;
		static const AVSampleFormat audio_sample_format_out = AV_SAMPLE_FMT_FLTP;
		static const AVSampleFormat audio_sample_format_in = AV_SAMPLE_FMT_S16;

		AVCodecContext* audio_codec_context = nullptr;
		SwrContext* audio_converter_context = nullptr;

		int audio_destination[SWR_CH_MAX];

		const uint8_t* audio_stream_buffer[SWR_CH_MAX];
		uint8_t* audio_packet_buffer[SWR_CH_MAX];
		uint8_t* audio_packet_buf = nullptr;

		int in_buffer_size, out_buffer_size, audio_packet_buffer_size, bufreq;
		// Audio		

	private:
		void init_video()
		{
			const AVCodec* video_encoder = avcodec_find_encoder(video_codec);
			if (!video_encoder)
				throw std::runtime_error("Video codec not found!");

			video_codec_context = avcodec_alloc_context3(video_encoder);
			video_codec_context->width = out_width;
			video_codec_context->height = out_height;
			video_codec_context->time_base.num = 1;
			video_codec_context->time_base.den = fps;
			video_codec_context->pix_fmt = H264_pixel_format;

			// H.264
			av_dict_set(&video_options, "preset", "ultrafast", 0);
			av_dict_set(&video_options, "tune", "zerolatency", 0);
			// H.264

			if (avcodec_open2(video_codec_context, video_encoder, &video_options) < 0)
				throw std::runtime_error("Cannot open video codec!");

			video_converter_context = sws_getContext(
				in_width, in_height, SDL_pixel_format,
				out_width, out_height, H264_pixel_format,
				SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
			);

			rgb_frame = av_frame_alloc();
			rgb_frame->width = in_width;
			rgb_frame->height = in_height;
			rgb_frame->format = SDL_pixel_format;

			yuv_frame = av_frame_alloc();
			yuv_frame->width = out_width;
			yuv_frame->height = out_height;
			yuv_frame->format = H264_pixel_format;

			const auto yuv_buffer_size = out_width * out_height * 3 / 2;
			yuv_buffer = new uint8_t[yuv_buffer_size];

			video_packet_buffer_size = in_width * in_height * channels;
			video_packet_buffer = new uint8_t[video_packet_buffer_size];

			avpicture_fill(
				reinterpret_cast<AVPicture*>(yuv_frame), yuv_buffer,
				H264_pixel_format,
				out_width, out_height);
		}

		void init_audio()
		{
			const AVCodec* audio_encoder = avcodec_find_encoder(audio_codec);
			if (!audio_encoder)
				throw std::runtime_error("Audio codec not found!");

			// alloc encoder
			audio_codec_context = avcodec_alloc_context3(audio_encoder);
			audio_codec_context->sample_fmt = audio_sample_format_out;
			audio_codec_context->sample_rate = out_sample_rate;
			audio_codec_context->channels = 2;
			audio_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
			audio_codec_context->time_base.num = 1;
			audio_codec_context->time_base.den = out_sample_rate;

			// AAC
			av_dict_set(&audio_options, "profile", "aac_low", 0);
			av_dict_set(&audio_options, "aac_coder", "fast", 0);
			// AAC

			if (avcodec_open2(audio_codec_context, audio_encoder, &audio_options) < 0)
				throw std::runtime_error("Cannot open audio codec!");

			// estimate sizes
			in_buffer_size = av_samples_get_buffer_size(
				nullptr,
				2,
				audio_codec_context->frame_size, // check
				audio_sample_format_in,
				1 /*no-alignment*/
			);

			out_buffer_size = av_samples_get_buffer_size(
				audio_destination,
				2,
				audio_codec_context->frame_size,
				audio_sample_format_out,
				1 /*no-alignment*/
			);

			/////////////////////////////////////////////////////////
			audio_converter_context = swr_alloc_set_opts(
				nullptr,
				AV_CH_LAYOUT_STEREO,
				audio_sample_format_out,
				out_sample_rate,
				AV_CH_LAYOUT_STEREO,
				audio_sample_format_in,
				in_sample_rate,
				0,
				nullptr
			);

			swr_init(audio_converter_context);

			// allocate resample buffer
			audio_packet_buffer_size = av_rescale_rnd(
				samples,
				out_sample_rate,
				in_sample_rate,
				AV_ROUND_UP
			);

			bufreq = av_samples_get_buffer_size(
				nullptr,
				2,
				audio_packet_buffer_size * 2,
				audio_sample_format_out,
				1 /*no-alignment*/
			);

			for (auto i = 0; i < SWR_CH_MAX; i++)
			{
				audio_packet_buffer[i] = nullptr;
				audio_stream_buffer[i] = nullptr;
			}

			audio_packet_buf = new uint8_t[bufreq];
			sound_in_frame = av_frame_alloc();
		}

	public:
		encode_to_mp4(const int in_width, const int in_height,
					  const int out_width, const int out_height,
					  const int channels, const int fps) :
			in_width(in_width),
			in_height(in_height),
			out_width(out_width),
			out_height(out_height),
			channels(channels),
			fps(fps)
		{
			av_register_all();
			avcodec_register_all();

			init_video();
			init_audio();
		}

		~encode_to_mp4()
		{
			//Video
			sws_freeContext(video_converter_context);

			avcodec_close(video_codec_context);
			av_free(video_codec_context);

			av_dict_free(&video_options);

			av_frame_unref(rgb_frame);
			av_frame_unref(yuv_frame);

			delete[] yuv_buffer;
			delete[] video_packet_buffer;

			//Audio
			swr_free(&audio_converter_context);

			avcodec_close(audio_codec_context);
			av_free(audio_codec_context);

			av_dict_free(&audio_options);

			av_frame_unref(sound_in_frame);
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
		 * \param ws_stream
		 * \return success
		 */
		bool add_instant(const uint8_t* audio_stream, const std::shared_ptr<std::ostream>& ws_stream)
		{
			audio_stream_buffer[0] = audio_stream;
			audio_packet_buffer[0] = audio_packet_buf;

			auto ret = swr_convert(
				audio_converter_context,
				audio_packet_buffer, audio_packet_buffer_size, // output
				audio_stream_buffer, samples // input
			);

			sound_in_frame->nb_samples = audio_codec_context->frame_size;
			sound_in_frame->format = audio_codec_context->sample_fmt;
			sound_in_frame->channel_layout = audio_codec_context->channel_layout;

			ret = avcodec_fill_audio_frame(
				sound_in_frame,
				audio_codec_context->channels,
				audio_codec_context->sample_fmt,
				audio_packet_buffer[0] /*samples+offset*/,
				audio_packet_buffer_size /*encoder_size*/,
				1 /*no-alignment*/
			);

			av_init_packet(&audio_packet);
			audio_packet.data = audio_packet_buffer[0];
			audio_packet.size = audio_packet_buffer_size;

			ret = avcodec_encode_audio2(audio_codec_context, &audio_packet, sound_in_frame, &got_packet_ptr);

			if (got_packet_ptr)
				ws_stream->write(reinterpret_cast<const char*>(video_packet.data), video_packet.size);

			return got_packet_ptr;
		}

		/**
		 * \brief Add video frame
		 * \param pixels
		 * \param ws_stream
		 * \return success
		 */
		bool add_frame(uint8_t* pixels, const std::shared_ptr<std::ostream>& ws_stream)
		{
			avpicture_fill(
				reinterpret_cast<AVPicture*>(rgb_frame),
				pixels,
				SDL_pixel_format,
				in_width, in_height);

			//RGB to YUV
			sws_scale(
				video_converter_context,
				rgb_frame->data, rgb_frame->linesize, 0, in_height,
				yuv_frame->data, yuv_frame->linesize
			);

			av_init_packet(&video_packet);
			video_packet.data = video_packet_buffer;
			video_packet.size = video_packet_buffer_size;

			avcodec_encode_video2(video_codec_context, &video_packet, yuv_frame, &got_packet_ptr);

			if (got_packet_ptr)
				ws_stream->write(reinterpret_cast<const char*>(video_packet.data), video_packet.size);

			return got_packet_ptr;
		}

	};
} // namespace encoding
