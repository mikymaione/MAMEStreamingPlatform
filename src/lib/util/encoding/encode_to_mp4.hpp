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
		//Video
		static const AVPixelFormat h264PixelFormat = AV_PIX_FMT_YUV420P;
		static const AVPixelFormat SDLPixelFormat = AV_PIX_FMT_RGB24;
		//SDL_PIXELFORMAT_ARGB32 = AV_PIX_FMT_RGB32		

		const int in_width, in_height, out_width, out_height, channels, fps;

		AVPacket avpkt;
		AVFrame* m_pRGBFrame = nullptr;
		AVFrame* m_pYUVFrame = nullptr;
		AVDictionary* opts = nullptr;
		AVDictionary* audio_opts = nullptr;
		AVCodecContext* c = nullptr;
		AVCodecContext* in_c = nullptr;
		SwsContext* scxt = nullptr;
		uint8_t* yuv_buff = nullptr;
		uint8_t* pkt_buff = nullptr;

		int pkt_buff_size, got_packet_ptr;
		//Video

	private:
		//Audio
		static const int SWR_CH_MAX = 32;
		static const int out_sample_rate = 48000; //44100;
		static const AVSampleFormat AudioSampleFormat = AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16; //AV_SAMPLE_FMT_U8

		AVCodecContext* encoder = nullptr;
		AVCodecContext* encoder_sdp = nullptr;
		SwrContext* swrctx = nullptr;

		int dstlines[SWR_CH_MAX];
		int source_size = -1;
		int encoder_size = -1;

		const uint8_t* stream_buf[SWR_CH_MAX];
		uint8_t* audio_buf[SWR_CH_MAX];
		int audio_buf_samples = 0;
		//Audio

	private:
		void initVideo()
		{
			auto pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);
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
				throw std::runtime_error("Cannot open codec H.264!");

			scxt = sws_getContext(
				in_width, in_height, SDLPixelFormat,
				out_width, out_height, h264PixelFormat,
				SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
			);

			m_pRGBFrame = av_frame_alloc();
			m_pRGBFrame->width = in_width;
			m_pRGBFrame->height = in_height;
			m_pRGBFrame->format = SDLPixelFormat;

			m_pYUVFrame = av_frame_alloc();
			m_pYUVFrame->width = out_width;
			m_pYUVFrame->height = out_height;
			m_pYUVFrame->format = h264PixelFormat;

			auto outbuf_size = out_width * out_height * 3 / 2;
			yuv_buff = new uint8_t[outbuf_size];

			pkt_buff_size = in_width * in_height * channels;
			pkt_buff = new uint8_t[pkt_buff_size];

			avpicture_fill((AVPicture*)m_pYUVFrame, yuv_buff, h264PixelFormat, out_width, out_height);
		}

		void initAudio()
		{
			auto pCodecAAC = avcodec_find_encoder(AV_CODEC_ID_AAC);
			if (!pCodecAAC)
				throw std::runtime_error("AAC codec not found!");

			// alloc encoder
			encoder = avcodec_alloc_context3(pCodecAAC);
			encoder->sample_fmt = AudioSampleFormat;
			encoder->sample_rate = out_sample_rate;
			encoder->channels = 2;
			encoder->channel_layout = AV_CH_LAYOUT_STEREO;
			encoder->time_base.num = 1;
			encoder->time_base.den = out_sample_rate;

			// need ctx with CODEC_FLAG_GLOBAL_HEADER flag
			encoder_sdp = avcodec_alloc_context3(pCodecAAC);
			encoder_sdp->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			encoder_sdp->sample_fmt = AudioSampleFormat;
			encoder_sdp->sample_rate = out_sample_rate;
			encoder_sdp->channels = 2;
			encoder_sdp->channel_layout = AV_CH_LAYOUT_STEREO;
			encoder_sdp->time_base.num = 1;
			encoder_sdp->time_base.den = out_sample_rate;

			av_dict_set(&audio_opts, "profile", "aac_low", 0);
			av_dict_set(&audio_opts, "aac_coder", "fast", 0);

			if (avcodec_open2(encoder, pCodecAAC, &audio_opts) < 0)
				throw std::runtime_error("Cannot open codec AAC!");

			if (avcodec_open2(encoder_sdp, pCodecAAC, &audio_opts) < 0)
				throw std::runtime_error("Cannot open codec AAC!");

			// estimate sizes
			source_size = av_samples_get_buffer_size(
				nullptr,
				2,
				encoder->frame_size,
				AudioSampleFormat,
				1 /*no-alignment*/
			);

			encoder_size = av_samples_get_buffer_size(
				dstlines,
				encoder->channels,
				encoder->frame_size,
				encoder->sample_fmt,
				1 /*no-alignment*/
			);
		}

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

			initVideo();
			initAudio();
		}

		~encode_to_mp4()
		{
			//Video
			sws_freeContext(scxt);

			avcodec_close(in_c);
			avcodec_close(c);

			av_free(in_c);
			av_free(c);

			av_dict_free(&opts);

			av_frame_unref(m_pRGBFrame);
			av_frame_unref(m_pYUVFrame);

			delete[] yuv_buff;
			delete[] pkt_buff;

			//Audio
			swr_free(&swrctx);

			avcodec_close(encoder);
			av_free(encoder);

			avcodec_close(encoder_sdp);
			av_free(encoder_sdp);

			av_dict_free(&audio_opts);
		}

		bool addIstant(uint8_t* stream, int in_sample_rate, int samples, std::shared_ptr<std::ostream> ws_stream)
		{
			if (swrctx == nullptr)
			{
				swrctx = swr_alloc_set_opts(
					nullptr,
					AV_CH_LAYOUT_STEREO,
					AudioSampleFormat,
					out_sample_rate,
					AV_CH_LAYOUT_STEREO,
					AudioSampleFormat,
					in_sample_rate,
					0,
					nullptr
				);

				swr_init(swrctx);

				// allocate resample buffer
				audio_buf_samples = av_rescale_rnd(
					samples,
					out_sample_rate,
					in_sample_rate,
					AV_ROUND_UP
				);

				int bufreq = av_samples_get_buffer_size(
					nullptr,
					2,
					audio_buf_samples * 2,
					AudioSampleFormat,
					1 /*no-alignment*/
				);

				for (size_t i = 0; i < SWR_CH_MAX; i++)
				{
					audio_buf[i] = nullptr;
					stream_buf[i] = nullptr;
				}

				audio_buf[0] = new uint8_t[bufreq];
			}

			stream_buf[0] = stream;

			swr_convert(
				swrctx,
				audio_buf, audio_buf_samples,
				stream_buf, samples
			);

			return true;
		}

		bool addFrame(uint8_t* pixels, std::shared_ptr<std::ostream> ws_stream)
		{
			avpicture_fill((AVPicture*)m_pRGBFrame, pixels, SDLPixelFormat, in_width, in_height);

			//RGB to YUV
			sws_scale(
				scxt,
				m_pRGBFrame->data, m_pRGBFrame->linesize, 0, in_height,
				m_pYUVFrame->data, m_pYUVFrame->linesize
			);

			av_init_packet(&avpkt);
			avpkt.data = pkt_buff;
			avpkt.size = pkt_buff_size;

			avcodec_encode_video2(c, &avpkt, m_pYUVFrame, &got_packet_ptr);

			if (got_packet_ptr)
				ws_stream->write((const char*)avpkt.data, avpkt.size);

			return got_packet_ptr;
		}

	};
} // namespace encoding
