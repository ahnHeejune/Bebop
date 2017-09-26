/* processing video from bebop : depenendent upon application  */

#include <opencv2/opencv.hpp>
#include "Bebop.h"

using namespace std;
using namespace cv;


static volatile bool stopVisionReq;
static void *vision_run(void *arg);
//BebopVision::start()
int vision_start(Bebop *pDrone) 
{ 
    pthread_t tidVision;

		stopVisionReq = false;
		return pthread_create(&tidVision, NULL, vision_run, pDrone);
}

int vision_stop()
{
	  stopVisionReq = true;
		return 0;
} 

//========================================================================
unsigned char sharedImgBuf[1284*240];
//========================================================================
static void *vision_run(void *arg)
{

  Bebop *pDrone = (Bebop *) arg; 
	static bool needRecording = false;
  const char *winName = "bebop";

	VideoWriter *recVid = NULL;
  float fps = 25.0;
  Size sz; 

  if(needRecording){
		 recVid  = new VideoWriter("record.avi",CV_FOURCC('M','J','P','G'), fps, sz);
		if(!recVid)
			std:cerr << "error in creating Video writer" << std::endl;
	}

	namedWindow(winName, CV_WINDOW_AUTOSIZE);
  Mat img = cv::Mat(240, 428,CV_8UC3, sharedImgBuf, 1284);
  //Mat img = Mat(240,428, CV_8UC3, Scalar(250, 250, 250));
	while(!stopVisionReq){

    if(pDrone->nImageCount){
			imshow(winName, img);
  		if(needRecording){
				recVid->write(img);
			}
      pDrone->nImageCount = 0; // used it 
		}
		waitKey(10); // @TODO, this should not get any key since we have another thread to getting key input
	}

  std::cerr << "Finishing Vision" << std::endl;
 	if(needRecording){
		recVid->release();
		free(recVid);
	} 
  cvDestroyWindow(winName); 
  //destroyAllWindows(); 
	waitKey(10);
  return (void *)1;
}


