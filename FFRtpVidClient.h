#ifndef FFRTPVIDCLIENT_H
#define FFRTPVIDCLIENT_H

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
//#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}

#include "RtpVidClient.h"

// abstract class for RTP video client 
// independent upon library 
class FFRtpVidClient : public RtpVidClient 
{
protected:

	AVFormatContext *fmt_ctx;
	AVCodecContext *video_dec_ctx;
	AVStream *video_stream;
	const char *src_filename;
	char *video_dst_filename;
	FILE *video_dst_file;
	uint8_t *video_dst_data[4];
	int      video_dst_linesize[4];
	int video_dst_bufsize;
	int video_stream_idx;
	AVFrame *frame;
	AVPacket pkt;
	int video_frame_count;
  struct SwsContext *sws_ctx;
	int scale;

	int decode_packet(int *got_frame, int cached);
	int open_codec_context(int *stream_idx,
       AVFormatContext *fmt_ctx, enum AVMediaType type);

public:
	FFRtpVidClient(); 
	int open(const char *sdp);
	int decode_frame();
	void close();
	int  getResolution(int *pWidth, int *pHeight, float *pFps);  
	int  getImagePtr(unsigned char **pptr, int *pstride);  
};
#endif


