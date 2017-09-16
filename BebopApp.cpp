/*
 * bebop.cpp
 *
 */

#include <pthread.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "Bebop.h"
#include <termios.h>


/* keyboard input handling */

static void setiNonBlockTerminal()
{
	struct termios ctrl;
	tcgetattr(STDIN_FILENO, &ctrl);
	ctrl.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

}

static int waitKey(int delayInms = 10)
{
	char ckey;
	fd_set s_rd;
	struct timeval delay = {0L, delayInms*1000L};  
	int fd = STDIN_FILENO;

	FD_ZERO(&s_rd);
	FD_SET(fd, &s_rd);
	int n = select(fd+1, &s_rd, NULL, NULL, &delay); 
	if(n < 0){
		std::cerr << "error in select" << std::endl;
	}else if(n > 0){
		if(FD_ISSET(fd, &s_rd)){
			std::cin >> ckey;
			return (int) ckey;
			//std::cout << "key:" << ckey << std::endl;
		}else{
			std::cerr << "select from unknow fd" << std::endl;
			return -1;
		}
	} // else timeout
        return -1; // 0?

}

static void showCommands()
{

	std::cout << "  ============================================" << std::endl;
	std::cout << " Bebop 2017 " << std::endl;
	std::cout << " - t:take-off, s:landiing, v: video-toggle" << std::endl;
	std::cout << " - h: hovering, u:up, d:down " << std::endl;
	std::cout << " - f:fowwards, b:backwards, r:right, l:left,c:clockwise, r:counter-clockwise" << std::endl;
	std::cout << " - c:clockwise, x:counter-clockwise" << std::endl;
	std::cout << " - longer commands to be defined" << std::endl;
	std::cout << "  ============================================" << std::endl;

} 

int main(int argc, char *argv[])
{
	int key; 
	int result;
  Bebop drone;
  char curCmd = (char)0; // nothing
  bool videoEnabled = false;
  int pcmdVal = 5; //5%


	showCommands();
	setiNonBlockTerminal();
	
	usleep(1000000);
     	result = drone.start();
	if(result < 0) return -1;

	int n = 0;
	while(1){
		if(n++ % 100 == 0)
			std::cout << n++ << std::endl;

/*
		if(n == 500)
			drone.mediaStreamingEnable(true);
		else if(n == 1000)
			drone.mediaStreamingEnable(false);
		else if(n == 1500)
			drone.mediaStreamingEnable(true);
		else if(n == 2000)
			drone.mediaStreamingEnable(false);

*/
		// takeoff test, be carefull 
/*		else if(n == 700)
			drone.takeoff();
		else if(n == 900)
			drone.land();
		else if(n == 1000)
			drone.emergency();
*/
			

#if 0 
		//  only work when widnows  
		if((key = cvWaitKey(30)) ==  'q'){
			break;
		}
		if(key < 0) continue;
		std::cout <<  "key-in:" << key << std::endl;
		if(key == 'q'){
			break;
		}

#elif 1

		key = waitKey(10);
		if(key >= 0){

			std::cout << "KEY:" << (char)key << std::endl;
			switch((char)key){

			case 'v':
				if(!videoEnabled){
					drone.mediaStreamingEnable(true);
					videoEnabled = true;
				}else{
					drone.mediaStreamingEnable(false);
					videoEnabled = false;

				}
                                curCmd = key;
				break;

			// MODE

			case 't':
				drone.takeoff();
        curCmd = key;
				break;
			case 's':
				drone.land();
        curCmd = key;
				break;

			// NAV
			case 'f':
				drone.setPCMD(0, -pcmdVal, 0, 0); // pitch < 0  
        curCmd = key;
				break;
			case 'b':
				drone.setPCMD(0, pcmdVal, 0, 0);  // pitch > 0
        curCmd = key;
				break;
			case 'r':
				drone.setPCMD(pcmdVal, 0, 0, 0);  // roll > 0 
        curCmd = key;
				break;
			case 'l':
				drone.setPCMD(-pcmdVal, 0, 0, 0); // roll < 0 
        curCmd = key;
				break;
			case 'u':
				drone.setPCMD(0, 0, -pcmdVal, 0);
        curCmd = key;
				break;
			case 'd':
				drone.setPCMD(0, 0, pcmdVal, 0);
        curCmd = key;
				break;
			case 'c':
				drone.setPCMD(0, 0, 0, pcmdVal);
        curCmd = key;
				break;
			case 'x':
				drone.setPCMD(0, 0, 0,-pcmdVal);
        curCmd = key;
				break;
			case 'q':
       	drone.emergency();
				usleep(1000000);
        curCmd = key;
			  goto _finish_point;
			  break;

			default:
				
				std::cerr << "undefined command" << std::endl;
		}// switch

	     } // key
#endif
	}


_finish_point:

    	result = drone.stop();
	if(result < 0) return -1;

	return 0;

}


