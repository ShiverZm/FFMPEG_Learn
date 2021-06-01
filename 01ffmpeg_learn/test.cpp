#include <stdio.h>


#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavutil/imgutils.h>
#include <SDL2/SDL.h>
}

#include "Window.h"
using namespace std;

static void decode(AVCodecContext* dec_ctx, AVPacket* pkt, AVFrame* pFrame, AVFrame* yuvFrame, struct SwsContext* imgCtx, FILE* outfile)
{
	int ret;
	/* send the packet with the compressed data to the decoder */
	ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0)
	{
		fprintf(stderr, "Error submitting the packet to the decoder\n");
		exit(1);
	}
	/* read all the output frames (in general there may be any number of them */
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(dec_ctx, pFrame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0)
		{
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}
		cout << "decoding the frame " << dec_ctx->frame_number << endl;
		sws_scale(imgCtx, pFrame->data, pFrame->linesize, 0, dec_ctx->height, yuvFrame->data, yuvFrame->linesize);
		int y_size = dec_ctx->width * dec_ctx->height;
		fwrite(yuvFrame->data[0], 1, y_size, outfile);		// Y 
		fwrite(yuvFrame->data[1], 1, y_size / 4, outfile);	// U
		fwrite(yuvFrame->data[2], 1, y_size / 4, outfile);	// V
	}
}


int DecodeH264ToYUV(const char* H264FileName, const char* YUVFileName)
{
		//SDL---------------------------
		int screen_w = 0, screen_h = 0;
		SDL_Window* screen;
		SDL_Renderer* sdlRenderer;
		SDL_Texture* sdlTexture;
		SDL_Rect sdlRect;
		//SDL---------------------------

		FILE* fp_yuv = NULL;
		fopen_s(&fp_yuv, YUVFileName, "wb+");
		// 打开h264文件，并把文件信息存入fctx中 
		int iRes = 0;
		AVFormatContext* fctx = avformat_alloc_context();
		if ((iRes = avformat_open_input(&fctx, H264FileName, NULL, NULL)) != 0)
		{
			cout << "File open failed!" << endl;
			return -1;
		}
		// 寻找视频流信息
		if (avformat_find_stream_info(fctx, NULL) < 0)
		{
			cout << "Stream find failed!\n";
			return -1;
		}
		// dump调试信息
		av_dump_format(fctx, -1, H264FileName, NULL);
		// 打开了视频并且获取了视频流 ，设置视频索引值默认值
		int vindex = -1;
		for (int i = 0; i < fctx->nb_streams; i++)
		{
			if (fctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
				vindex = i;
		}
		// 如果没有找到视频的索引，说明并不是一个视频文件
		if (vindex == -1)
		{
			cout << "Codec find failed!" << endl;
			return -1;
		}
		// 分配解码器上下文空间
		AVCodecContext* cctx = avcodec_alloc_context3(NULL);
		// 获取编解码器上下文信息
		if (avcodec_parameters_to_context(cctx, fctx->streams[vindex]->codecpar) < 0)
		{
			cout << "Copy stream failed!" << endl;
			return -1;
		}
		// 查找解码器
		AVCodec* c = avcodec_find_decoder(cctx->codec_id);
		if (!c) {
			cout << "Find Decoder failed!" << endl;
			return -1;
		}
		// 打开解码器
		if (avcodec_open2(cctx, c, NULL) != 0) {
			cout << "Open codec failed!" << endl;
			return -1;
		}
		// 对图形进行宽度上方的裁剪，以便于显示得更好
		struct SwsContext* imgCtx = sws_getContext(cctx->width, cctx->height, cctx->pix_fmt,
			cctx->width, cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		if (!imgCtx)
		{
			cout << "Get swscale context failed!" << endl;
			return -1;
		}

		//SDL Init----------------------
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
		{
			cout << "Could not initialize SDL - " << SDL_GetError() << endl;
			return -1;
		}
		screen_w = cctx->width;
		screen_h = cctx->height;
		//SDL 2.0 Support for multiple windows
		screen = SDL_CreateWindow("FFmpeg Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL);
		if (!screen) {
			cout << "SDL: could not create window - exiting:" << SDL_GetError() << endl;
			return -1;
		}
		sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
		sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, cctx->width, cctx->height);
		sdlRect.x = 0;
		sdlRect.y = 0;
		sdlRect.w = screen_w;
		sdlRect.h = screen_h;
		//SDL Init End----------------------

		// 初始化已解码帧
		AVPacket* pkt = av_packet_alloc();
		AVFrame* pFrame = av_frame_alloc();
		// 初始化解码得到的yuv帧
		AVFrame* yuvFrame = av_frame_alloc();
		int vsize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, cctx->width, cctx->height, 1);
		uint8_t* buf = (uint8_t*)av_malloc(vsize);
		av_image_fill_arrays(yuvFrame->data, yuvFrame->linesize, buf, AV_PIX_FMT_YUV420P, cctx->width, cctx->height, 1);
		//char errbuf[256] = { 0 };
		// 循环读取每一帧
		while (av_read_frame(fctx, pkt) >= 0)
		{
			if (pkt->stream_index == vindex)
			{
				/*
				if ((iRes = avcodec_send_packet(cctx, pkt)) != 0)
				{
					cout << "Send video stream packet failed!" << endl;
					av_strerror(iRes, errbuf, 256);
					return -5;
				}
				if ((iRes = avcodec_receive_frame(cctx, pFrame)) != 0)
				{
					cout << "Receive video frame failed!" << endl;
					av_strerror(iRes, errbuf, 256);
					return -6;
				}
				cout << "decoding the frame " << cctx->frame_number << endl;
				sws_scale(imgCtx, pFrame->data, pFrame->linesize, 0, cctx->height, yuvFrame->data, yuvFrame->linesize);
				int y_size = cctx->width*cctx->height;
				fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);		// Y
				fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);	// U
				fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);	// V
				*/
				decode(cctx, pkt, pFrame, yuvFrame, imgCtx, fp_yuv);
				//SDL-----------------------------------------------------------------------------------------------------
				SDL_UpdateYUVTexture(sdlTexture, &sdlRect, yuvFrame->data[0], yuvFrame->linesize[0],
					yuvFrame->data[1], yuvFrame->linesize[1], yuvFrame->data[2], yuvFrame->linesize[2]);
				SDL_RenderClear(sdlRenderer);
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
				SDL_RenderPresent(sdlRenderer);
				SDL_Delay(40);
				//SDL End--------------------------------------------------------------------------------------------------
			}
		}
		/* flush the decoder */
		pkt->data = NULL;
		pkt->size = 0;
		decode(cctx, pkt, pFrame, yuvFrame, imgCtx, fp_yuv);
		//SDL---------------------------------------------------------------------------------------------------------------
		SDL_UpdateYUVTexture(sdlTexture, &sdlRect, yuvFrame->data[0], yuvFrame->linesize[0],
			yuvFrame->data[1], yuvFrame->linesize[1], yuvFrame->data[2], yuvFrame->linesize[2]);
		SDL_RenderClear(sdlRenderer);
		SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
		SDL_RenderPresent(sdlRenderer);
		SDL_Delay(40);
		SDL_Quit();
		//SDL End-----------------------------------------------------------------------------------------------------------
		//释放内存
		av_free(buf);
		av_frame_free(&yuvFrame);
		av_frame_free(&pFrame);
		av_packet_free(&pkt);
		sws_freeContext(imgCtx);
		avcodec_free_context(&cctx);
		avformat_close_input(&fctx);
		avformat_free_context(fctx);
		return 0;
}

int main()
{
	Window::Init("h264 show");

	char* szFilePath = "..\\..\\res\\01ffmpeg_learn\\bigbuckbunny_480x272.h264";


	const char* H264FileName = "../H264/test.h264";
	const char* YUVFileName = "./YUV/test.yuv";
	const char* YUVFilesName = "../YUV/frame";
	if (DecodeH264ToYUV(szFilePath, YUVFileName) < 0)
		cout << "Decode failed!" << endl;
	cout << "Decode finished!" << endl;
	return getchar();
}