/*------------------------------------------------------------------------------
 * bebop.cpp
 *
 * Bebop Demo Application 
 *
 * - provide basic test commands 
 * - 1: forwrds constant speed
 * - 2: turn 90 degree
 * - 3: curvature (radius?)   lengthof1 / pi/2 / time_of_2 
 *-----------------------------------------------------------------------------
 */

#include <pthread.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "Bebop.h"
#include <termios.h>
#include <cmath>
#include <algorithm>


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


//===================================================================
//PTcontroll or drone 
//
// the current implementaiton is as follows:
//
//  - limited proportional to target - currrent

//                   30   +-------------- 
//                       /    
//                      /
//      --------------/-------------------->
//                  / 0 
//                /
//      --------+ -30
//
// Kpx : linear [-0.3, +0.3] m/s  
// kpy : linear [-0.15, +0.15] m/s 
// Kpa : linear [-30, +30] degree
// Kpz : linear [-1.0, +1.0] meter
// 
// @TODO move to BebopControl after verification  
//===================================================================
static void controlDrone(Bebop &drone, float target[4])
{
    const float Kpx = 100.0, Kpy = 50.0, Kpz = 30.0,  Kpa = 60.0;
	  int  cmdRoll, cmdPitch, cmdSpeedYaw, cmdGaz;
    float curVx = -drone.getSpeedX(); // due to x-axis def of bebop
    float curVy = -drone.getSpeedY();
    float curYaw = drone.getYaw();
    float curAlt = drone.getAlt();
    float tarVx  = target[0];
    float tarVy  = target[1];
    float tarYaw  = target[2];
    float tarAlt  = target[3];

    // 0. only send command when flying state
    DRONE_STATE droneState = drone.getFlyingState();
    if(droneState != HOVERING && droneState != FLYING) // only safei condition 
				return;
     
    // 1. control output only P-controller 
    cmdPitch = (int)Kpx*(tarVx - curVx);
    cmdRoll  = (int)Kpy*(tarVy - curVy);
    cmdSpeedYaw = (int)Kpa*(tarYaw - curYaw);
    cmdGaz = (int)Kpz*(tarAlt - curAlt);

    // 2. limit not to operate  extremely    
    cmdPitch = std::max(std::min(cmdPitch,30), -30);  
    cmdRoll  = std::max(std::min(cmdRoll,30), -30); 
    cmdSpeedYaw = std::max(std::min(cmdSpeedYaw,30), -30);  
    cmdGaz = std::max(std::min(cmdGaz,30), -30);  

#if 0  // for debugging 
    std::cerr << "Control:x:" << tarVx << ","  << curVx << "=>" << cmdPitch << std::endl;  
    std::cerr << "Control:y:" << tarVy << ","  << curVy << "=>" << cmdRoll  << std::endl;  
    std::cerr << "Control:a:" << tarYaw << ","  << curYaw << "=>" << cmdSpeedYaw << std::endl;  
    std::cerr << "Control:z:" << tarAlt << ","  << curAlt << "=>" << cmdGaz << std::endl;  
#endif

    // set new control values
    drone.setPCMD(cmdRoll, cmdPitch, cmdSpeedYaw, cmdGaz);
}

static void showCommands()
{

	std::cout << "  ============================================" << std::endl;
	std::cout << " Bebop 2017 " << std::endl;
	std::cout << " - t:take-off, s:landiing, v: video-toggle" << std::endl;
	std::cout << " - h: hovering, u:up, d:down " << std::endl;
	std::cout << " - f:fowwards, b:backwards, r:right, l:left,c:clockwise, r:counter-clockwise" << std::endl;
	std::cout << " - c:clockwise, x:counter-clockwise" << std::endl;
	std::cout << " - controlled mode:  1: forwared  2: rotation 3:  curve turn" << std::endl;
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
  int initialPcmdVal = 10;
  int pcmdVal = initialPcmdVal; //10%
	float targets[4];  // target vx, vy, yaw_angle, altitude 
  float curYaw;

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

		key = waitKey(25);
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

			// simple NAV
			case 'f':  // @TODO: increase when press one more.
				if(curCmd == 'f')   
				    pcmdVal += 5;  // speed up
				else 
				    pcmdVal = initialPcmdVal;
        			drone.setPCMD(0, pcmdVal, 0, 0); // pitch < 0  
					curCmd = key;
				break;
			case 'b':
				if(curCmd == 'b')   
				    pcmdVal += 5;  // speed up
				else 
				    pcmdVal = initialPcmdVal;
				drone.setPCMD(0, -pcmdVal, 0, 0);  // pitch > 0
        curCmd = key;
				break;
			case 'r':
				if(curCmd == 'r')   
				    pcmdVal += 5;  // speed up
				else 
				    pcmdVal = initialPcmdVal;
				drone.setPCMD(pcmdVal, 0, 0, 0);  // roll < 0 
        			curCmd = key;
				break;
			case 'l':
				drone.setPCMD(-pcmdVal, 0, 0, 0); // roll > 0 
        			curCmd = key;
				break;
			case 'u':
				drone.setPCMD(0, 0, 0, pcmdVal);
        			curCmd = key;
				break;
			case 'd':
				drone.setPCMD(0, 0, 0, -pcmdVal);
        			curCmd = key;
				break;
			case 'c':
				if(curCmd == 'c')   
				    pcmdVal += 5;  // speed up
				else 
				    pcmdVal = initialPcmdVal;
				drone.setPCMD(0, 0, pcmdVal, 0);
        			curCmd = key;
				break;
			case 'x':
				if(curCmd == 'x')   
				    pcmdVal += 5;  // speed up
				else 
				    pcmdVal = initialPcmdVal;
				drone.setPCMD(0, 0, -pcmdVal, 0);
        			curCmd = key;
				break;
					
			case 'h': 
				drone.setPCMD(0, 0, 0, 0);
        curCmd = key;
				break;


      // high level control
      case '1':  // straight forward constan speed 
        curCmd = key;  
        curYaw = drone.getYaw();
			  targets[0] =  0.3; // vx, + : forwards, - : backwards
		    targets[1] =  0.0; // vy, +: right      - : left 
			  targets[2] =  curYaw; // angle  
			  targets[3] =  1.0; // meter, + : up, -dn 
				break;
      case '2':  // turn 90 degree 
        curCmd = key;  
        curYaw = drone.getYaw();
			  targets[0] = 0.0; // vx
		    targets[1] = 0.0; // vy
			  targets[2] = fmod(curYaw + M_PI/2, 2.0*M_PI) ; // angle  
			  if(targets[2] > M_PI)  targets[2] -= M_PI;  
			  targets[3] = 1.0; // meter 
				break;
      case '3':  // curve
        curCmd = key;  
			  targets[0] = 0.3; // vx
		    targets[1] = 0.0; // vy
			  targets[2] = fmod(curYaw + M_PI/2, 2.0*M_PI) ; // angle  
			  if(targets[2] > M_PI)  targets[2] -= M_PI;  
			  targets[3] = 1.0; // meter 
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


    // motion control
		if(curCmd == '1' || curCmd == '2' || curCmd == '3'){
			controlDrone(drone, targets);
		}

#endif
	}


_finish_point:

    	result = drone.stop();
	if(result < 0) return -1;

	return 0;

}


