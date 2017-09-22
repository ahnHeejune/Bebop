#ifndef RTPVIDCLIENT_H
#define RTPVIDCLIENT_H

// abstract class for RTP video client 
// independent upon library 
class RtpVidClient 
{
protected:
   int width;
   int height;
   float fps;
   
public:
	virtual int open(const char *sdp) = 0;
	virtual int decode_frame() = 0;
	virtual void close() = 0;
	virtual int  getResolution(int *pWidth, int *pHeight, float *pfps ) = 0;  
	virtual int  getImagePtr(unsigned char **pptr, int *pstride) = 0;  

  static  RtpVidClient *getInstance();
};
#endif




