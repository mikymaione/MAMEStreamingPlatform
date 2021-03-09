// license:BSD-3-Clause
// copyright-holders:Michele Maione
//============================================================
//
//  encode_to_mp4.h - Encoding to MP4 (H.264 + AAC) (aka DinoEncoding 🦕🧡🦖)
//
//============================================================

#pragma once

#ifndef SRC_ENCODETOMP4_H
#define SRC_ENCODETOMP4_H

#include "libavcodec/avcodec.h"

#include "libavutil/frame.h"
#include "libavutil/imgutils.h"

namespace encoding
{
	class encode_to_mp4
	{
	private:
		const int frame_per_seconds = 15;

	private:
		void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, char *buffer)
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

				printf("encoded frame %3" PRId64 " (size=%5d)\n", pkt->pts, pkt->size);
				fwrite(pkt->data, 1, pkt->size, buffer);
				av_packet_unref(pkt);
			}
		}

	public:
		void encode_frame()
		{
			/* find the H.264 encoder */
			avcodec_register(AV_CODEC_ID_H264);
			const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
			if (!codec)
			{
				fprintf(stderr, "codec not found\n");
				exit(1);
			}

			AVCodecContext *c = avcodec_alloc_context3(codec);
			AVFrame *picture = av_frame_alloc();
			AVPacket *pkt = av_packet_alloc();
			if (!pkt)
				exit(1);

			/* put sample parameters */
			c->bit_rate = 8 * 2; // 2KB
			/* resolution must be a multiple of two */
			c->width = 352;
			c->height = 288;
			/* frames per second */
			c->time_base = (AVRational){1, frame_per_seconds};
			c->framerate = (AVRational){frame_per_seconds, 1};

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

			char *f;

			/* encode 1 second of video */
			for (int i = 0; i < frame_per_seconds; i++)
			{
				fflush(stdout);

				/* make sure the frame data is writable */
				ret = av_frame_make_writable(picture);
				if (ret < 0)
					exit(1);

				/* prepare a dummy image */
				/* Y */
				for (int y = 0; y < c->height; y++)
					for (int x = 0; x < c->width; x++)
						picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;

				/* Cb and Cr */
				for (int y = 0; y < c->height / 2; y++)
					for (int x = 0; x < c->width / 2; x++)
					{
						picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
						picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
					}

				picture->pts = i;

				/* encode the image */
				encode(c, picture, pkt, f);
			}

			/* flush the encoder */
			encode(c, NULL, pkt, f);

			/* add sequence end code to have a real MPEG file */
			uint8_t endcode[] = {0, 0, 1, 0xb7};
			fwrite(endcode, 1, sizeof(endcode), f);

			avcodec_free_context(&c);
			av_frame_free(&picture);
			av_packet_free(&pkt);
		}
	};
} // namespace encoding

#endif //SRC_ENCODETOMP4_H