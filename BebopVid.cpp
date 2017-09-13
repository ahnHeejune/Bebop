/* RTP video stream decoder using openCV */


#include <opencv2/opencv.hpp>
#include "ARConstant.h"
#include "Bebop.h"

using namespace std;
using namespace cv;


//========================================================================
// bebop media control 
// 
// video status:  stopped, startReq, running, stopReq
// 
// @TODO: have to separate the control and video part?
//========================================================================
bool Bebop::mediaStreamingEnable(bool enable) 
{
        //@TODO, check status with enable 

	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_MEDIASTREAMING;

	unsigned int size = 12;
	//frame_t pframe;
	unsigned char command[13];
	memset(command, 0, 13);

	command[0] = ARConstant::ARNETWORKAL_FRAME_TYPE_DATA; //pframe.type;
	command[1] = ARConstant::BD_NET_CD_NONACK_ID; //pframe.id;
	command[2] = seq[ARConstant::BD_NET_CD_NONACK_ID]++; //pframe.seq;

	char Byte1 = (size & 0xff000000) >> 24;
	char Byte2 = (size & 0xff0000) >> 16;
	char Byte3 = (size & 0xff00) >> 8;
	char Byte4 = (size & 0xff);

	command[3] = Byte4;
	command[4] = Byte3;
	command[5] = Byte2;
	command[6] = Byte1;

	command[7] = _project;
	command[8] = _class;
	command[9] = 0;
	command[10] = 0;
	if (enable)
		command[11] = 1;
	else
		command[11] = 0;

	// 2. send
//	unsigned char txbuf[12] = {0x02, 0x0a, 0x03, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x15, 0x00, 0x00, 0x01};
        
	//int n = rio_writen(c2dSock, txbuf, 12); //command, 12);
	int n = send2Drone((unsigned char *)&command, 12);
	if(n != 12){
		std::cout << "Error in tx media-enable  :" << n << std::endl;
		return false;
	}

	if(enable){ // 4. run the drone agent thread
		std::cout << "Media Enabling..."<<  std::endl;
		stopVidReq = false;
		pthread_create(&tidVid, NULL, runVid, this);
	}else{
		std::cout << "Media Disabling..."<<  std::endl;
		stopVidReq = true;
		// wait for joining the thread?

		// set state = stopped 
	}
	return true;
}


//========================================================================
// RTP RX using OpenCV (internally ffmpeg) 
// @TODO: have to separate the control and video part?
//========================================================================
void *Bebop::innerRunVid()
#if 1 
{
  const char *winName = "rtp";
	//use path of "bebop.sdp" file
	const string sdpLocation = "./bebop.sdp";

	std::cout << "==========video thread============================="<< std::endl;

	Mat image;
	VideoCapture *vcap = new VideoCapture(sdpLocation);
	if(!vcap->isOpened()){
		std::cerr << "error in openning video " << std::endl;
		vcap->release();
		return NULL;
	}

	namedWindow(winName, CV_WINDOW_AUTOSIZE);
	while(!stopVidReq){
		if(!vcap->read(image))	
        break;
     image.copyTo(img); // thread safe clone for using it 
        
		imshow(winName, image);
		waitKey(10);
	}

  std::cerr << "Finishing RTP" << std::endl;
  //cvDestroyWindow(winName); 
  destroyAllWindows(); 
	waitKey(10);
	vcap->release();
  return (void *)1;
}
#else // for testing video is comming 
{
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket < 0){
                std::cerr << "rtp socket() error" << std::endl;
                return (void *)-1;
        }

        struct sockaddr_in myaddr;
        memset(&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_port = htons(55004);
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0){
                std::cerr << "d2csocket bind() error" << std::endl;
                return (void *)-2;
        }


	int nread = 0;
        int count = 0;
	char buf[2048];
        stopVidReq = false;


	std::cerr << "Start Video stream " << std::endl;
	while(!stopVidReq){

        	if ((nread = read(sock, buf, 2048)) < 0) {
			std::cerr << "read error " << std::endl;
		}else{
			count++;
		//	std::cerr << "read : " << nread  << std::endl;
		}
	}

	std::cerr << "Stopped Video stream : "  << count << std::endl;

    	stopVidReq = false; // auto-reset 
    	return (void *)1;

}
#endif

