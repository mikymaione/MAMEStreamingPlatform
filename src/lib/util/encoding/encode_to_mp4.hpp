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

// standard SDL headers
#include <SDL2/SDL.h>

// FFMPEG C headers
extern "C"
{
#include "libavcodec/avcodec.h"

#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
}

namespace encoding
{
	class encode_to_mp4
	{
	private:
		const int frame_per_seconds = 30;

		//AVCodec ff_h264_encoder;

		AVFrame* picture;
		AVCodecContext* c;
		AVPacket* pkt;

	private:
		void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt, std::shared_ptr<std::ostream> append_stream)
		{
			/* send the frame to the encoder */
			int ret = avcodec_send_frame(enc_ctx, frame);
			if (ret < 0)
			{
				fprintf(stderr, "error sending a frame for encoding\n");
				exit(1);
			}

			while (ret >= 0)
			{
				ret = avcodec_receive_packet(enc_ctx, pkt);

				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					return;
				}
				else if (ret < 0)
				{
					fprintf(stderr, "error during encoding\n");
					exit(1);
				}

				//fwrite(pkt->data, 1, pkt->size, dest);
				append_stream->write((const char*)pkt->data, pkt->size);

				av_packet_unref(pkt);
			}
		}

	public:
		encode_to_mp4()
		{
			//avcodec_register(&ff_h264_encoder);
			avcodec_register_all();

			/* find the H.264 encoder */
			const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!codec)
			{
				fprintf(stderr, "codec not found\n");
				exit(1);
			}

			c = avcodec_alloc_context3(codec);
			picture = av_frame_alloc();
			pkt = av_packet_alloc();
			if (!pkt)
				exit(1);

			/* resolution must be a multiple of two */
			// 360p
			c->width = 480;
			c->height = 360;
			c->bit_rate = 770000; //0.77 Mb/s

			/* frames per second */
			c->time_base = { 1, frame_per_seconds };
			c->framerate = { frame_per_seconds, 1 };

			c->pix_fmt = AV_PIX_FMT_YUV420P;

			/* open it */
			if (avcodec_open2(c, codec, NULL) < 0)
			{
				fprintf(stderr, "could not open codec\n");
				exit(1);
			}

			picture->format = c->pix_fmt;
			picture->width = c->width;
			picture->height = c->height;

			int ret = av_frame_get_buffer(picture, 32);
			if (ret < 0)
			{
				fprintf(stderr, "could not alloc the frame data\n");
				exit(1);
			}
		}

		void stop(std::shared_ptr<std::ostream> append_stream)
		{
			/* flush the encoder */
			encode(c, NULL, pkt, append_stream);

			/* add sequence end code to have a real MPEG file */
			uint8_t endcode[] = { 0, 0, 1, 0xb7 };

			//fwrite(endcode, 1, sizeof(endcode), f);
			append_stream->write((const char*)endcode, sizeof(endcode));

			avcodec_free_context(&c);
			av_frame_free(&picture);
			av_packet_free(&pkt);
		}

		void encode_frame(int64_t n_frame, SDL_Surface* surf, std::shared_ptr<std::ostream> append_stream)
		{
			fflush(stdout);

			/* make sure the frame data is writable */
			int ret = av_frame_make_writable(picture);
			if (ret < 0)
				exit(1);

			Uint8* pixel = (Uint8*)surf->pixels;
			Uint8 val;

			for (int y = 0; y < surf->h; y++)
				for (int x = 0; x < surf->w; x++)
				{
					// R
					val = *pixel;
					picture->data[0][y * picture->linesize[0] + x] = val;
					pixel++;

					// G
					val = *pixel;
					picture->data[1][y * picture->linesize[1] + x] = val;
					pixel++;

					// B
					val = *pixel;
					picture->data[2][y * picture->linesize[2] + x] = val;
					pixel++;
				}

			picture->pts = n_frame;

			/* encode the image */
			encode(c, picture, pkt, append_stream);
		}
	};
} // namespace encoding
