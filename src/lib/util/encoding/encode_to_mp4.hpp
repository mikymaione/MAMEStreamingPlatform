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
		const int width;
		const int height;
		const int channels;
		const int fps;

		AVFrame* m_pRGBFrame = new AVFrame[1];
		AVFrame* m_pYUVFrame = new AVFrame[1];
		AVCodec* pCodecH264 = nullptr;
		AVDictionary* opts = nullptr;
		AVCodecContext* c = nullptr;
		AVCodecContext* in_c = nullptr;
		SwsContext* scxt = nullptr;
		uint8_t* yuv_buff = nullptr;
		uint8_t* outbuf = nullptr;

		int outbuf_size;

	public:
		encode_to_mp4(const int width, const int height, const int channels, const int fps) :
			width(width), height(height), channels(channels), fps(fps)
		{
			av_register_all();
			avcodec_register_all();

			pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!pCodecH264)
				throw std::runtime_error("H.264 codec not found!");

			c = avcodec_alloc_context3(pCodecH264);
			c->bit_rate = 3000000;
			c->width = width;
			c->height = height;

			// frames per second 			
			c->time_base.num = 1;
			c->time_base.den = fps;
			c->gop_size = 10; // emit one intra frame every ten frames 
			c->max_b_frames = 1;
			c->thread_count = 1;
			c->pix_fmt = AV_PIX_FMT_YUV420P;

			av_dict_set(&opts, "profile", "baseline", 0);
			av_dict_set(&opts, "preset", "superfast", 0);
			av_dict_set(&opts, "tune", "zerolatency", 0);
			av_dict_set(&opts, "intra-refresh", "1", 0);
			av_dict_set(&opts, "slice-max-size", "1500", 0);

			if (avcodec_open2(c, pCodecH264, &opts) < 0)
				throw std::runtime_error("Cannot open codec");

			int size = c->width * c->height * 3 / 2;
			outbuf_size = c->width * c->height * channels;

			yuv_buff = new uint8_t[size];
			outbuf = new uint8_t[outbuf_size];

			scxt = sws_getContext(c->width, c->height, AV_PIX_FMT_RGB32, c->width, c->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
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
			delete[] outbuf;
			delete[] yuv_buff;
		}

		bool addFrame(void* pixels, std::shared_ptr<AVPacket> avpkt)
		{
			avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)pixels, AV_PIX_FMT_RGB32, width, height);

			//YUV buffer to YUV Frame
			avpicture_fill((AVPicture*)m_pYUVFrame, (uint8_t*)yuv_buff, AV_PIX_FMT_YUV420P, width, height);

			// RGB
			/*
			m_pRGBFrame->data[0] += m_pRGBFrame->linesize[0] * (height - 1);
			m_pRGBFrame->linesize[0] *= -1;
			m_pRGBFrame->data[1] += m_pRGBFrame->linesize[1] * (height / 2 - 1);
			m_pRGBFrame->linesize[1] *= -1;
			m_pRGBFrame->data[2] += m_pRGBFrame->linesize[2] * (height / 2 - 1);
			m_pRGBFrame->linesize[2] *= -1;
			*/

			//RGB to YUV
			sws_scale(scxt, m_pRGBFrame->data, m_pRGBFrame->linesize, 0, c->height, m_pYUVFrame->data, m_pYUVFrame->linesize);

			av_init_packet(avpkt.get());
			avpkt->data = outbuf;
			avpkt->size = outbuf_size;

			int got_packet_ptr;
			avcodec_encode_video2(c, avpkt.get(), m_pYUVFrame, &got_packet_ptr);

			return got_packet_ptr;
		}

	};
} // namespace encoding
