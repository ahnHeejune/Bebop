/*
 * bebop.h
 *
 *  Created on: 2017. 8. 8.
 *      Author: lee
 */

#ifndef BEBOP_H_
#define BEBOP_H_

#ifndef EXTERN
#define EXTERN extern
#endif
#include <iostream>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <opencv2/opencv.hpp> // because of 
#include <time.h>

//Pack memory
#pragma pack(push)
#pragma pack(1)
typedef
struct {
  unsigned char type;  	// 1B
  unsigned char id;   		// 1B
  unsigned char seq;   	// 1B
  unsigned int  size;  	// 4B
  unsigned char data[1024]; // max 1024B ?
} frame_t;
#pragma  pack(pop)

// CMD Header 2
//Pack memory
#pragma pack(push)
#pragma pack(1)
typedef struct {
		unsigned char project;	// 1B
		unsigned char clazz;	// 1B
		unsigned short cmd;		// 2B
		unsigned char params[4];// dummy
} cmd_t;


typedef enum {
	Takeoff = 0,
	Land, //1
	Stop,//2
	Emergency,//3
	Auto,//4
	Forward,//5
	Clockwise,//6
	Right,//7
	Manual,//8
	Backward,//9
	CounterClockwise,//10
	Left,//11
	Up,//12
	Down,//13
	ForceStop, //14
	ForceClockwise,
	ForceCounterClockwise,
	NONE//15
} NAV_CMD_TYPE;


#pragma  pack(pop)

#pragma pack(push)
#pragma pack(1)
struct navcmd
{
//public:

   unsigned char flag; // Boolean flag to activate roll/pitch movement
   unsigned char roll; // roll Roll consign for the drone [-100;100]
   unsigned char pitch;// Pitch consign for the drone [-100;100]
   unsigned char yawSpeed;  // yaw Yaw consign for the drone [-100;100]
   unsigned char zSpeed;  // gaz Gaz consign for the drone [-100;100]
   float psi;          //  [NOT USED] - Magnetic north heading of the
  		               //  controlling device (deg) [-180;180]
   navcmd()
   {
	   flag = 1;
	   roll = 0;
	   pitch = 0;
	   yawSpeed = 0;
	   zSpeed = 0;
	   psi = 0.0; // not using
   }
};
#pragma  pack(pop)

typedef enum
{
	LANDED = 0,     // initial and landded  
	TAKINGOFF = 1,  // on take-off command 
	HOVERING  = 2,  // on finishing taking off
	FLYING = 3,
	LANDING = 4,    // on land command 
	EMERGENCY,
	DRONE_STATE_MAX // not connected
} DRONE_STATE;

//-----------------------------------------------------------------------------
// bebop proxy class
//-----------------------------------------------------------------------------
class Bebop
{
protected:
    // state
    DRONE_STATE droneState;
    unsigned char seq[256];

    // sockets ---------------------------------------//
    int d2cSock, c2dSock;
    pthread_mutex_t c2dMutex;
    int discoverDrone(const char* peerIp); // discovery using TCP
    int setupc2dSocket(const char* droneIP, int dronePort);
    // no close?
    int setupd2cSocket(short controlPort);
    // no close? 

    //------------------------------------------------// 
    float speedX, speedY, speedZ;
    float roll, yaw, pitch;
		float altitudeBaro;  // altitude from ground, not GPS 
public:
    DRONE_STATE getFlyingState()
		{
			   return droneState;
		}
    float getSpeedX()
		{
				return speedX;
		}
    float getSpeedY()
		{
				return speedY;
		}
    float getSpeedZ()
		{
				return speedZ;
		}
    float getRoll()
		{
				return roll;
		}
    float getYaw()
		{
				return yaw;
		}
    float getPitch()
		{
				return pitch;
		}
    float getAlt()
		{
				return (float)altitudeBaro;
		}
     
protected:
    NAV_CMD_TYPE  mNavigationCommand;
    unsigned char inputParam;
    static unsigned char validateAngle(unsigned char angle);
    
    double latitude, longitude, altitudeGPS;

public:
    float getLat()
		{
				return (float)latitude;
		}
    float getLon()
		{
				return (float)longitude;
		}
protected:
 
 
    // log-related -------------------------------// 
    static const char* LOG_DIR_PATH;
    static const char* LOG_FILE_PREFIX;
    pthread_mutex_t logMutex;
    timeval startTime;
    unsigned long elapsedTime();
    //FILE* videoDecode;//videoDecode.txt // Save the video decode time
    //FILE* visionProcessFile;//visionProcess.txt // Save the time of Video decoding, Canndy detection, Hough line, Vanishing , Tracking, SendCMD2Control
    //FILE* lineSegmentFile;//lineSegment.txt
    //FILE* visionFile;//resultVision.txt
    // Logs GPS
    FILE* logFptr;
    timeval* globalClock;
    int  logInit();
    int  logClose();
    bool logSetFly(int mode);
    bool logSetPCMD(int pitch, int roll, int yawSpeed, int gaz); // pitch(x)-roll(y)
    bool logFlyingStatus(int status);
    bool logSetMedia(int status);
    bool logMediaStatus(int status);
    bool logGPSInfo(double latitude, double longitude, double altitude); // GPS degree
    bool logSpeedInfo(float speedx, float speedy, float speedz); // m/s
    bool logAttitudeInfo(float pitch, float roll, float yaw); // radian
    bool logAltitudeInfo(double altitude); // meter
    bool logUndefinedMsg(int pjt, int clazz, int cmd, unsigned char *msg, int prntlen);


    cv::Mat img; // current image
    int onVideoFrame(unsigned char *data, int size);
    //int h264DecoderDecode(unsigned char *inbuf, int len, bool toSave);


    //  XX related ----------------------------//
    //received data
    static float readFloat(unsigned char *buf);
    static double readDouble(unsigned char *buf);
    void onD2CEventFrame(unsigned char *buf, int len); // for change of drone status  
    void onD2CNavFrame(unsigned char *buf, int len);   // for navigation data  
    void onD2CFrame();
    static int readData(int fd, void *usrbuf, size_t n);
    
    //  TX related ----------------------------//
    //static ssize_t rio_writen(int fd, void *usrbuf, size_t n);
    ssize_t send2Drone(unsigned char *usrbuf, size_t n);
    int sendAck(frame_t *pinframe);
    int sendPong(frame_t *pping);
    void controlDroneCmdGenerator(unsigned char _project, unsigned char _class, unsigned char _command);

    void buildPCMD(navcmd navcmd, unsigned char size, unsigned char *returnBuf);
    void sendNavCmd(NAV_CMD_TYPE cmdType, unsigned char value);
    navcmd pcmd; 
    int sendPCMD(); // send 
    void findAllState();
    int  waitControlEvent(int ms);
    

    //Thread management ------------------------//
    pthread_t tid, tidVid;
    // control thread
    volatile bool stopControlReq;
    void *innerRun();
    static void *run(void *inst){
	((Bebop *)inst)->stopControlReq = false;
	return ((Bebop *)inst)->innerRun();
    }
    // video thread
    volatile bool stopVidReq;
    void *innerRunVid();
    static void *runVid(void *inst){
	((Bebop *)inst)->stopVidReq = false;
	return ((Bebop *)inst)->innerRunVid();
    }
    int stopVid( ){
	stopVidReq = true;
    }

    
public:
    
    Bebop();
    ~Bebop();  
    // flight mode control
    void flattrim();
    void takeoff();  // void ? not ack?
    void land();     // void ? not ack?  
    void emergency();// void ? not ack?
    
    // turn on or off video streaming 
    bool mediaStreamingEnable(bool enable);

    // navigation control
    int setPCMD(int roll, int pitch, int yawSpeed, int zSpeed);
	 
/*
    navcmd upCMD(unsigned char val);   // +z
    navcmd downCMD(unsigned char val); // -z
    navcmd forwardCMD(unsigned char val);  // +x
    navcmd backwardCMD(unsigned char val);  // -x
    navcmd rightCMD(unsigned char val);  // +y
    navcmd leftCMD(unsigned char val);  // -y
    navcmd clockwiseCMD(unsigned char val);  // +r
    navcmd counterClockwiseCMD(unsigned char val);  // -r
*/
    
    // start communication 
    int start( ); 
    // @TODO: before stop communication, it should land-on the system
    int stop( ){
	stopControlReq = true;
    }

    
    
};


#endif /* BEBOP_H_ */
