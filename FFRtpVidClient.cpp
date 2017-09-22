/**
 * from ffmpeg  doc/examples/demuxing.c
 */

#include <iostream>
#include "FFRtpVidClient.h"
 
/* 
  decode one pkt maybe one RTP frame? 
  cached: if new pkt data or not 
  ret:  number of bytes?
  got_frame: if new frame or not
*/
int FFRtpVidClient::decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    if (pkt.stream_index == video_stream_idx) {
        /* decode video frame */
        ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame\n");
            return ret;
        }
        if (*got_frame) {
#if  0 
            printf("video_frame%s n:%d coded_n:%d pts:%s\n",
                   cached ? "(cached)" : "",
                   video_frame_count++, frame->coded_picture_number,
                   av_ts2timestr(frame->pts, &video_dec_ctx->time_base));

            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize,
                          (const uint8_t **)(frame->data), frame->linesize,
                          video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
            /* write to rawvideo file */
            fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
#endif
        }
    } else{ // if (pkt.stream_index == audio_stream_idx) {
        // ignore audio
    }
    return ret;
}


/* start video rtp rx */
int FFRtpVidClient::open_codec_context(int *stream_idx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                "av_get_media_type_string(type)", this->src_filename);
                //av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    "av_get_media_type_string(type)");
            return ret;
        }
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    "av_get_media_type_string(type)");
            return ret;
        }
    }
    return 0;
}

FFRtpVidClient::FFRtpVidClient()
{ 
    static bool isFirst = true;
    if(isFirst == true){
        isFirst = false;
	      av_register_all();
		}
 
		fmt_ctx = NULL;
		video_dec_ctx = NULL;
    src_filename = "./bebop.sdp";
		video_stream = NULL;
    video_dst_filename = (char *)"video.raw";
		video_dst_file = NULL;
		memset(video_dst_data, 0, 4); // image for raw video   
		memset(video_dst_linesize, 0, 4);
		video_dst_bufsize;
		video_stream_idx = -1;
		frame = NULL;                  // decoded frame
		//ffrtp->pkt;                         // rtp packets
		scale = 2;
}


//=============================================================================
// waitng get some information from RTP 
// should get some RTP packets enough to codec starts  
//
//=============================================================================
int FFRtpVidClient::open(const char *sdpLocation) 
{
   int ret;
   src_filename = sdpLocation; 

    /* 1. open input file (now, SDP file), and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    /* 2. retrieve stream information, waiting some RTP packets  */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    // 3. now we know wich decoder needed
    // prepare decoder
    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dec_ctx = video_stream->codec;
#if 0 
        this->video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }
#endif

      // converter YUV420 to RGB
    	sws_ctx = sws_getContext(
														video_dec_ctx->width,
														video_dec_ctx->height,
														video_dec_ctx->pix_fmt, //AV_PIX_FMT_YUVJ420P,
														video_dec_ctx->width/this->scale, // reduce 
														video_dec_ctx->height/this->scale, //
#ifndef AV_PIX_FMT_RGB24
                             //PIX_FMT_RGB24,
                             PIX_FMT_BGR24,
#else
                             AV_PIX_FMT_RGB24,
                             //AV_PIX_FMT_BGR24,
#endif
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL);

        /* 4. allocate image where the decoded image will be put */
        ret = av_image_alloc(video_dst_data, video_dst_linesize, // output 
                             video_dec_ctx->width/scale, video_dec_ctx->height/scale,
                             PIX_FMT_RGB24, 1);  // format and align
                             //this->video_dec_ctx->pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);
    if (!video_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }
    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    if (video_stream)
        printf("Demuxing video from file '%s' into '%s'\n", this->src_filename, this->video_dst_filename);
    if (this->video_stream) {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
               av_get_pix_fmt_name(video_dec_ctx->pix_fmt), video_dec_ctx->width, video_dec_ctx->height,
               video_dst_filename);
    }

    return 0;

end:  // when something wrong happens
    if (video_dec_ctx)
        avcodec_close(video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (video_dst_file)
        fclose(video_dst_file);
    av_free(frame);
    av_free(video_dst_data[0]);

    //delete inst;
    return -1;
}


/* 

  return: -1, no more data 
           0, got frames (TODO: return YUV buffer?)     
*/
int FFRtpVidClient::decode_frame()
{
    int got_frame; 

    /* read frames from the file */
    // how long it will wait?
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        decode_packet(&got_frame, 0);
        av_free_packet(&this->pkt);
        if(got_frame) {
						std::cout << "nframe=" << video_frame_count++ << std::endl;
    				sws_scale(sws_ctx,
              frame->data, frame->linesize, 0, video_dec_ctx->height,
							video_dst_data, video_dst_linesize);
						return  1; //(void *) video_dst_data;
				}
		}
    return  -1; //NULL;
}


void FFRtpVidClient::close()
{
    int got_frame; 

  /* 1. flush cached frames */
  pkt.data = NULL;
  pkt.size = 0;
  do {
      decode_packet(&got_frame, 1);
      // ignore all the cached frames
  } while (got_frame);

#if 0 
  printf("Demuxing succeeded.\n");
  if (video_stream) {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
               av_get_pix_fmt_name(inst->video_dec_ctx->pix_fmt), inst->video_dec_ctx->width, inst->video_dec_ctx->height,
               inst->video_dst_filename);
    }
#endif

    // 2. free resources
    if (video_dec_ctx)
        avcodec_close(video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (video_dst_file)
        fclose(video_dst_file);
    av_free(frame);
    av_free(video_dst_data[0]);
    //av_freep(&dst_data[0]);
		if(sws_ctx)
    		sws_freeContext(sws_ctx);
}

int  FFRtpVidClient::getResolution(int *pWidth, int *pHeight, float *pFps)
{
    *pWidth = video_dec_ctx->width/scale;
    *pHeight = video_dec_ctx->height/scale;
    *pFps    =  29.0; // TODO video_dec_ctx->fps;
;
    return 0;
}

int  FFRtpVidClient::getImagePtr(unsigned char **pptr, int *pstride)
{
   *pptr = video_dst_data[0];
   *pstride = video_dst_linesize[0];
   return 0;
}

