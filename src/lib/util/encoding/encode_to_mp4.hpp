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
#include <vector>

// FFMPEG C headers
extern "C"
{
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
}

namespace encoding
{
	class encode_to_mp4
	{
	private:
		const int frame_per_seconds = 25;

		// 360p
		const unsigned int width = 480;
		const unsigned int height = 360;
		unsigned int iframe;

		SwsContext* swsCtx;
		AVOutputFormat* fmt;
		AVStream* stream;
		AVFormatContext* fc;
		AVCodecContext* c;
		AVPacket pkt;

		AVFrame* rgbpic, * yuvpic;

		std::vector<uint8_t> pixels;

	public:
		encode_to_mp4() :
			iframe(0), pixels(4 * width * height)
		{
			// Preparing to convert my generated RGB images to YUV frames.
			swsCtx = sws_getContext(width, height, AV_PIX_FMT_RGB24, width, height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

			// Preparing the data concerning the format and codec,
			// in order to write properly the header, frame data and end of file.			
			fmt = av_guess_format("mp4", nullptr, nullptr);

			//SOSTITUIRE
			//avformat_alloc_output_context2(&fc, nullptr, nullptr, filename.c_str());
			fc = avformat_alloc_context();

			// Setting up the codec.
			auto codec = avcodec_find_encoder(AV_CODEC_ID_H264);

			AVDictionary* opt = nullptr;
			av_dict_set(&opt, "preset", "veryfast", 0);
			av_dict_set(&opt, "crf", "20", 0);

			stream = avformat_new_stream(fc, codec);

			c = stream->codec;
			c->width = width;
			c->height = height;
			c->pix_fmt = AV_PIX_FMT_YUV420P;
			c->time_base = { 1, frame_per_seconds };

			// Setting up the format, its stream(s),
			// linking with the codec(s) and write the header.
			if (fc->oformat->flags & AVFMT_GLOBALHEADER)
				// Some formats require a global header.
				c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			avcodec_open2(c, codec, &opt);
			av_dict_free(&opt);

			// Once the codec is set up, we need to let the container know
			// which codec are the streams using, in this case the only (video) stream.
			stream->time_base = { 1, frame_per_seconds };

			//SOSTITUIRE
			//av_dump_format(fc, 0, filename.c_str(), 1);

			//SOSTITUIRE
			//avio_open(&fc->pb, filename.c_str(), AVIO_FLAG_WRITE);

			avformat_write_header(fc, &opt);
			av_dict_free(&opt);

			// Preparing the containers of the frame data:
			// Allocating memory for each RGB frame, which will be lately converted to YUV.
			rgbpic = av_frame_alloc();
			rgbpic->format = AV_PIX_FMT_RGB24;
			rgbpic->width = width;
			rgbpic->height = height;
			av_frame_get_buffer(rgbpic, 1);

			// Allocating memory for each conversion output YUV frame.
			yuvpic = av_frame_alloc();
			yuvpic->format = AV_PIX_FMT_YUV420P;
			yuvpic->width = width;
			yuvpic->height = height;
			av_frame_get_buffer(yuvpic, 1);

			// After the format, code and general frame data is set,
			// we can write the video in the frame generation loop:
			// std::vector<uint8_t> B(width*height*3);
		}

		~encode_to_mp4()
		{
			// Writing the delayed frames:
			for (auto got_output = 1; got_output; )
			{
				avcodec_encode_video2(c, &pkt, nullptr, &got_output);

				if (got_output)
				{
					av_packet_rescale_ts(&pkt, { 1, frame_per_seconds }, stream->time_base);

					pkt.stream_index = stream->index;

					av_interleaved_write_frame(fc, &pkt);
					av_packet_unref(&pkt);
				}
			}

			// Writing the end of the file.
			av_write_trailer(fc);

			// Closing the file.
			if (!(fmt->flags & AVFMT_NOFILE))
				avio_closep(&fc->pb);

			avcodec_close(stream->codec);

			// Freeing all the allocated memory:
			sws_freeContext(swsCtx);
			av_frame_free(&rgbpic);
			av_frame_free(&yuvpic);
			avformat_free_context(fc);
		}

		void addFrame(const uint8_t* pixels)
		{
			// The AVFrame data will be stored as RGBRGBRGB... row-wise,
			// from left to right and from top to bottom.
			for (auto y = 0u; y < height; y++)
				for (auto x = 0u; x < width; x++)
				{
					// rgbpic->linesize[0] is equal to width.
					rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 0] = pixels[y * 4 * width + 4 * x + 2];
					rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 1] = pixels[y * 4 * width + 4 * x + 1];
					rgbpic->data[0][y * rgbpic->linesize[0] + 3 * x + 2] = pixels[y * 4 * width + 4 * x + 0];
				}

			// Not actually scaling anything, but just converting
			// the RGB data to YUV and store it in yuvpic.
			sws_scale(swsCtx, rgbpic->data, rgbpic->linesize, 0, height, yuvpic->data, yuvpic->linesize);

			av_init_packet(&pkt);
			pkt.data = nullptr;
			pkt.size = 0;

			// The PTS of the frame are just in a reference unit,
			// unrelated to the format we are using. We set them,
			// for instance, as the corresponding frame number.
			yuvpic->pts = iframe;

			int got_output;
			avcodec_encode_video2(c, &pkt, yuvpic, &got_output);

			if (got_output)
			{
				// We set the packet PTS and DTS taking in the account our FPS (second argument),
				// and the time base that our selected format uses (third argument).
				av_packet_rescale_ts(&pkt, { 1, frame_per_seconds }, stream->time_base);

				pkt.stream_index = stream->index;

				// Write the encoded frame to the mp4 file.
				av_interleaved_write_frame(fc, &pkt);
				av_packet_unref(&pkt);
			}
		}

	};
} // namespace encoding
