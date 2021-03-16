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
		const AVPixelFormat h264PixelFormat = AV_PIX_FMT_YUV420P;
		const AVPixelFormat SDLPixelFormat = AV_PIX_FMT_RGB24;
		//SDL_PIXELFORMAT_ARGB32 = AV_PIX_FMT_RGB32		

		const int in_width, in_height, out_width, out_height, channels, fps;

		AVFrame* m_pRGBFrame = new AVFrame[1];
		AVFrame* m_pYUVFrame = new AVFrame[1];
		AVCodec* pCodecH264 = nullptr;
		AVDictionary* opts = nullptr;
		AVCodecContext* c = nullptr;
		AVCodecContext* in_c = nullptr;
		SwsContext* scxt = nullptr;
		uint8_t* yuv_buff = nullptr;
		uint8_t* pkt_buff = nullptr;

		int pkt_buff_size;

	public:
		encode_to_mp4(const int in_width, const int in_height, const int out_width, const int out_height, const int channels, const int fps) :
			in_width(in_width),
			in_height(in_height),
			out_width(out_width),
			out_height(out_height),
			channels(channels),
			fps(fps)
		{
			av_register_all();
			avcodec_register_all();

			pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!pCodecH264)
				throw std::runtime_error("H.264 codec not found!");

			c = avcodec_alloc_context3(pCodecH264);
			c->width = out_width;
			c->height = out_height;
			c->time_base.num = 1;
			c->time_base.den = fps;
			c->pix_fmt = h264PixelFormat;

			av_dict_set(&opts, "preset", "ultrafast", 0);
			av_dict_set(&opts, "tune", "zerolatency", 0);

			if (avcodec_open2(c, pCodecH264, &opts) < 0)
				throw std::runtime_error("Cannot open codec");

			auto outbuf_size = out_width * out_height * 3 / 2;
			yuv_buff = new uint8_t[outbuf_size];

			pkt_buff_size = in_width * in_height * channels;
			pkt_buff = new uint8_t[pkt_buff_size];

			scxt = sws_getContext(
				in_width, in_height, SDLPixelFormat,
				out_width, out_height, h264PixelFormat,
				SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
			);

			m_pRGBFrame->width = in_width;
			m_pRGBFrame->height = in_height;
			m_pYUVFrame->width = out_width;
			m_pYUVFrame->height = out_height;
		}

		~encode_to_mp4()
		{
			/*
			AVCodec* pCodecH264 = nullptr;
			AVDictionary* opts = nullptr;
			AVCodecContext* in_c = nullptr;
			SwsContext* scxt = nullptr;
			*/

			avcodec_close(c);
			av_free(c);
			av_dict_free(&opts);

			av_frame_unref(m_pRGBFrame);
			av_frame_unref(m_pYUVFrame);

			delete[] m_pRGBFrame;
			delete[] m_pYUVFrame;
			delete[] yuv_buff;
			delete[] pkt_buff;
		}

		bool addFrame(void* pixels, std::shared_ptr<AVPacket> avpkt)
		{
			avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)pixels, SDLPixelFormat, in_width, in_height);
			avpicture_fill((AVPicture*)m_pYUVFrame, yuv_buff, h264PixelFormat, out_width, out_height);

			//RGB to YUV
			sws_scale(
				scxt,
				m_pRGBFrame->data, m_pRGBFrame->linesize, 0, in_height,
				m_pYUVFrame->data, m_pYUVFrame->linesize
			);

			int got_packet_ptr;
			av_init_packet(avpkt.get());
			avpkt->data = pkt_buff;
			avpkt->size = pkt_buff_size;

			avcodec_encode_video2(c, avpkt.get(), m_pYUVFrame, &got_packet_ptr);

			return got_packet_ptr;
		}

	};
} // namespace encoding
