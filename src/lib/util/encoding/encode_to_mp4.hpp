// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - Encoding to MP4 (H.264 + AAC) (aka DinoEncoding 🦕🧡🦖)
//
//============================================================
#pragma once

#include <chrono>
#include <memory>
#include <ostream>

// FFMPEG C headers
extern "C"
{
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
}

namespace encoding
{
	class encode_to_mp4
	{
	private:
		const int channels = 4;
		const int frame_per_seconds = 25;

		// 360p
		const int width = 480;
		const int height = 360;

		AVFrame* pic_in;
		AVStream* video_stream;
		AVFormatContext* format_context;
		AVCodecContext* codec_context;
		SwsContext* sws_context;

		bool first_image_timestamp_setted = false;
		std::chrono::system_clock::time_point first_image_timestamp;

		unsigned char* io_buffer;

		std::ostream* append_stream;

	private:
		// output callback for ffmpeg IO context
		static int dispatch_output_packet(void* opaque, uint8_t* buffer, int buffer_size)
		{
			auto stream = (std::ostream*)opaque;
			stream->write((const char*)buffer, buffer_size);

			return 0;
		}

	public:
		encode_to_mp4(std::ostream* append_stream)
			: append_stream(append_stream)
		{
			const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);

			size_t io_buffer_size = 3 * 1024;    // 3M seen elsewhere and adjudged good
			io_buffer = new unsigned char[io_buffer_size];

			auto io_ctx = avio_alloc_context(io_buffer, io_buffer_size, AVIO_FLAG_WRITE, append_stream, nullptr, dispatch_output_packet, nullptr);
			io_ctx->seekable = 0;

			format_context = avformat_alloc_context();
			format_context->oformat = av_guess_format("mp4", nullptr, nullptr);
			format_context->pb = io_ctx;
			format_context->max_interleave_delta = 0;

			codec_context = avcodec_alloc_context3(codec);
			//codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
			codec_context->delay = 0;
			codec_context->time_base.num = 1;
			codec_context->time_base.den = 1;
			codec_context->width = width;
			codec_context->height = height;

			pic_in = av_frame_alloc();
			pic_in->width = width;
			pic_in->height = height;
			pic_in->format = AV_PIX_FMT_YUV420P;

			video_stream = avformat_new_stream(format_context, codec);

			video_stream->time_base.num = 1;
			video_stream->time_base.den = 1000;

			avcodec_parameters_from_context(video_stream->codecpar, codec_context);

			codec_context->flags |= AV_CODEC_FLAG_LOW_DELAY;
			codec_context->max_b_frames = 0;

			AVDictionary* opts = nullptr;
			av_dict_set(&opts, "profile", "baseline", 0);
			av_dict_set(&opts, "preset", "superfast", 0);
			av_dict_set(&opts, "tune", "zerolatency", 0);
			av_dict_set(&opts, "intra-refresh", "1", 0);
			av_dict_set(&opts, "slice-max-size", "1500", 0);

			avcodec_open2(codec_context, codec, &opts);

			sws_context = sws_getContext
			(
				width, height, AV_PIX_FMT_RGB32,
				width, height, AV_PIX_FMT_YUV420P,
				SWS_BICUBIC,
				nullptr, nullptr, nullptr
			);
		}

		~encode_to_mp4()
		{
			avcodec_close(codec_context);
			av_free(codec_context);
			av_free(pic_in);
		}

		void stop()
		{

		}

		void addFrame(void* pixels)
		{
			AVPicture raw_frame;

			avpicture_fill
			(
				&raw_frame,
				(const uint8_t*)pixels,
				AV_PIX_FMT_RGB32,
				width, height
			);

			auto ret = sws_scale(
				sws_context,
				raw_frame.data,
				raw_frame.linesize,
				0, height,
				pic_in->data, pic_in->linesize
			);

			AVPacket pkt;
			av_init_packet(&pkt);

			ret = avcodec_send_frame(codec_context, pic_in);

			if (ret == AVERROR_EOF)
				std::cout << "avcodec_send_frame() encoder flushed\n";
			else if (ret == AVERROR(EAGAIN))
				std::cout << "avcodec_send_frame() need output read out\n";

			if (ret < 0)
				throw std::runtime_error("Error encoding video frame");

			ret = avcodec_receive_packet(codec_context, &pkt);
			auto got_packet = (ret == AVERROR(EAGAIN) ? 0 : pkt.size > 0);

			if (got_packet)
			{
				auto stamp = std::chrono::system_clock::now();

				if (!first_image_timestamp_setted)
				{
					first_image_timestamp_setted = true;
					first_image_timestamp = stamp;
				}

				auto diff = (stamp - first_image_timestamp);
				auto seconds = diff.count();

				// Encode video at 1/0.95 to minimize delay
				pkt.pts = (seconds / av_q2d(video_stream->time_base) * 0.95);

				if (pkt.pts <= 0)
					pkt.pts = 1;

				pkt.dts = AV_NOPTS_VALUE;

				if (pkt.flags & AV_PKT_FLAG_KEY)
					pkt.flags |= AV_PKT_FLAG_KEY;

				pkt.stream_index = video_stream->index;

				if (av_write_frame(format_context, &pkt))
					throw std::runtime_error("Error when writing frame");

				//append_stream->write((const char*)pkt.data, pkt.size);
			}

			av_free(pkt.data);
			av_free_packet(&pkt);
			av_packet_unref(&pkt);
		}

	};
} // namespace encoding
