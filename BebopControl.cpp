#include <cstdio>
#include <pthread.h>
#include <errno.h>

#include "ARConstant.h"
#include "Bebop.h"
#define MAX_FRAME_SIZE 15000

//========================================================================
// Constructor 
// 
// set clock, setup log, int protocols variables 
//========================================================================
Bebop::Bebop()
{
   // log 
   gettimeofday(&startTime, NULL);
   logInit();
   pthread_mutex_init(&c2dMutex, NULL); // c2dMutex = PTHREAD_MUTEX_INITIALIZER; 
   // protocols
   droneState = DRONE_STATE_MAX;
   memset(seq, 1, sizeof(seq));
}

//========================================================================
// Destructor
// 
// close sockets, any memory allocated, video libraries 
//========================================================================
Bebop::~Bebop()
{
    close(d2cSock);
    close(c2dSock);	

    // created in setup 
 		int status = pthread_mutex_destroy(&c2dMutex);
    if(status < 0)
      std::cerr << "error in destroying c2d mutex" << std::endl;

    // if video is running 
    // stop it first 
    logClose();
}

//------------------------------------------------------------------------
// create TCP socket; send json request; get the response 
// return -1 (error), 0 (success)
//------------------------------------------------------------------------
int Bebop::discoverDrone(const char* destIP)
{
	// make a TCP connection to drone and send json message
	short destPort = 44444;
	struct hostent* hp;
	struct sockaddr_in droneIP;

	int discSock; 

	// 1. create  TCP socket 
	discSock = socket(PF_INET, SOCK_STREAM, 0);
	if(discSock == -1){
		std::cerr << "disc socket socket() error" << std::endl;
		return -2;
	}
	memset(&droneIP, 0, sizeof(droneIP));
	if(inet_aton(destIP, &(droneIP.sin_addr)) < 0)
		std::cerr << "disc socket inet_aton() error" << std::endl;
	droneIP.sin_port = htons(destPort);
	droneIP.sin_family = AF_INET;

	if (connect(discSock, (struct sockaddr*) &droneIP, sizeof(struct sockaddr_in)) < 0){
		std::cerr << "disc connect error to " <<  destIP << ":"  
					<< strerror(errno) << std::endl;
		close(discSock);
		return -3;
	}

         // 2. send request
	const char *jsoninfo = "{\"controller_type\":\"computer\",\"controller_name\":\"node-bebop\",\"d2c_port\":43210,\"arstream2_client_stream_port\":55004,\"arstream2_client_control_port\":55005}";
	int n = strlen(jsoninfo);

	std::cout << jsoninfo << std::endl;

	if(write(discSock, jsoninfo, n)<0){
		std::cerr << "jsoninfo write error" << std::endl;
		close(discSock);
		return -4;
	}

        // 3. response 
	char buftemp[1024];
	std::cout << "discovery Acked"<< std::endl;
	n = read(discSock, (void*) buftemp, 1023);
	buftemp[n-1] = 0;
	printf("%d, %s\n", n, buftemp);
	//std::cout << buftemp << std::endl;
	std::cout << "connected" << std::endl;
	droneState = LANDED; // now connected, so landed  

 	close(discSock);
	return 0;
}

//double Bebop::calculatePeriodOfTime(timeval startTime) //return ms
double calculatePeriodOfTime(timeval startTime) //return ms
{
	timeval currentTime;
	gettimeofday(&currentTime, 0);
	return (currentTime.tv_sec + 0.000001*currentTime.tv_usec - startTime.tv_sec - 0.000001*startTime.tv_usec)*1000;
}

/* Create UDP socket for RX */
int Bebop::setupd2cSocket(short controlPort)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket < 0){
		std::cerr << "d2csocket socket() error" << std::endl;
		return -1;
	}

	struct sockaddr_in myaddr;
	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(controlPort);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0){
		std::cerr << "d2csocket bind() error" << std::endl;
		return -2;
	}
	return sock;
}

/* Create UDP socket for TX */

int Bebop::setupc2dSocket(const char* droneIP, int dronePort)
{

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket < 0){
		std::cerr << "c2dsocket socket() error" << std::endl;
		return -1;
	}

	struct sockaddr_in droneAddr;
	memset(&droneAddr, 0, sizeof(droneAddr));
	droneAddr.sin_family = AF_INET;
	droneAddr.sin_port = htons(dronePort);
	if(inet_aton(droneIP, &droneAddr.sin_addr) == 0){
		std::cerr << "drone ip addr translation error" << std::endl;
		close(sock);
		return -2;
	}

	if(connect(sock, (struct sockaddr*)&droneAddr, sizeof(droneAddr))<0){
		std::cerr << "c2dsocket connect() error" << std::endl;
		return -3;
	}

	return sock;
}




/** ----------------------------------------------------------------------------
  send simple ack frame to drone
  for which type frames?
------------------------------------------------------------------------------*/

int Bebop::sendAck(frame_t *pinframe)
{
	char txbuf[1024];
	frame_t *pAck = (frame_t *)txbuf;

	// 1. build frame
	pAck->type = ARConstant::ARNETWORKAL_FRAME_TYPE_ACK;
	pAck->id   = pinframe->id + (ARConstant::ARNETWORKAL_MANAGER_DEFAULT_ID_MAX / 2);
	pAck->seq  = seq[pAck->id]++;
	pAck->size = 7 + 1;
	pAck->data[0] = pinframe->seq;

	// 2. send
	send2Drone((unsigned char *)txbuf, pAck->size);
	return 0;
}

/** -------------------------------------------------------------------------------
     Keep-alive: reply ping with poing

---------------------------------------------------------------------------------- */
int Bebop::sendPong(frame_t *pping)
{
	//char txbuf[1024];
	//frame_t *ppong = (frame_t *)txbuf;
	frame_t ppong;

	// 1. build frame
	ppong.type = ARConstant::ARNETWORKAL_FRAME_TYPE_DATA;
	ppong.id   = ARConstant::ARNETWORK_MANAGER_INTERNAL_BUFFER_ID_PONG;
	ppong.seq  = seq[pping->id]++;
	ppong.size = pping->size;  // must be same
	//ppong.data = new unsigned char[pping->size - 7 + 1];
	memcpy(ppong.data,pping->data, pping->size -7); // copy the payload

	// 2. send
	send2Drone((unsigned char *) &ppong, ppong.size);
	//ppong.data = NULL;
	return 0;
}


/* 1. one-time commands===================================================*/

/* 
   General control command: Take-OFF, LAND, EMERGENCY etc  

*/
void Bebop::controlDroneCmdGenerator(unsigned char _project, 
                unsigned char _class, unsigned char _command)
{
	const unsigned int size = 11;
	frame_t pframe;

	pframe.type = ARConstant::ARNETWORKAL_FRAME_TYPE_DATA; //pframe.type;
	pframe.id = ARConstant::BD_NET_CD_NONACK_ID; //pframe.id;
	pframe.seq = seq[ARConstant::BD_NET_CD_NONACK_ID]++; //pframe.seq;
	pframe.size = size;

	pframe.data[0] = _project;
	pframe.data[1] = _class;

	pframe.data[2] = _command;
	pframe.data[3] = 0;

	//2.Send
	int n = send2Drone((unsigned char *)&pframe, size);
	if(n < 0){
		std::cerr  << "Fails to send command " 
					<< strerror(errno) << std::endl;
	}else{
		std::cerr  << "Send command : " << n << std::endl;
	}
	return;
}

/* 1. Send all Drone info */
void Bebop::findAllState()
{
	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_COMMON;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_COMMON_CLASS_COMMON;
	unsigned char _cmd = ARConstant::ARCOMMANDS_ID_COMMON_COMMON_CMD_ALLSTATES;
	controlDroneCmdGenerator(_project,_class,_cmd);
}

/* 2. trim at landing */
void Bebop::flattrim()
{
	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTING;
	unsigned char _cmd = ARConstant::ARCOMMANDS_ID_ARDRONE3_PILOTING_CMD_FLATTRIM;

	//Flatrim
	controlDroneCmdGenerator(_project, _class, _cmd);
}

/* 3. Take off */
void Bebop::takeoff()
{
	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTING;
	unsigned char _cmd = ARConstant::ARCOMMANDS_ID_ARDRONE3_PILOTING_CMD_TAKEOFF;

	//Take off
	controlDroneCmdGenerator(_project, _class, _cmd);
}

/* landing */
void Bebop::land()
{
	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTING;
	unsigned char _cmd = ARConstant::ARCOMMANDS_ID_ARDRONE3_PILOTING_CMD_LANDING;

	//Land
	controlDroneCmdGenerator(_project, _class, _cmd);
}

/* emergency ?? */
void Bebop::emergency()
{
	unsigned char _project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
	unsigned char _class = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTING;
	unsigned char _cmd = ARConstant::ARCOMMANDS_ID_ARDRONE3_PILOTING_CMD_EMERGENCY;

	//Emergency mode
	controlDroneCmdGenerator(_project, _class, _cmd);
}

//------------------------------------------------------------------------
// Navigation Command Related 
//------------------------------------------------------------------------

/* check range: 0 to 100 % */
static unsigned char validatePercent(unsigned char value)  // unsinged 
{
	if(value > 100)
		return 100;
	else if (value < 0)
		return 0;
	return value | 0;
}

//----------------------------------------------------------------------
// check in range 
//   true : in range 
//   false: out of range
//----------------------------------------------------------------------
static bool inRange(int min, int value, int max) 
{
	if(value > max)
		return false;
	else if (value < min )
		return false;
	else 
		return  true;
}

int Bebop::sendPCMD()
{
	unsigned char commandSize = 20;
	unsigned char buf[commandSize + 1];

	// 1. build 
	buildPCMD(pcmd, commandSize, buf);
	// 2. Send command
	int n = send2Drone((unsigned char *)buf, commandSize);
	if(n < 0){
		std::cerr << "sendNavCmd write error" << std::endl;
	  	return -1; 
	}
	return 0; // success
}

#if 0 
/* up : thrust */
navcmd Bebop::upCMD(unsigned char val)
{
	navcmd cmd;
	cmd.gaz = validatePercent(val); // ??
	//navcmd.flag = 1;
	return cmd;
}
/* dn: thrust */
navcmd Bebop::downCMD(unsigned char val){
	navcmd cmd;
	cmd.gaz = validatePercent(val)*(-1);
	//navcmd.flag = 1;
	return cmd;
}

/* forward : by pitch */
navcmd Bebop::forwardCMD(unsigned char val)
{
	navcmd cmd;
	cmd.pitch = validatePercent(val);
	cmd.flag = 1;
	return cmd;
}

/* back : by pitch */
navcmd Bebop::backwardCMD(unsigned char val)
{
	navcmd cmd;
	cmd.pitch = validatePercent(val)*(-1);
	cmd.flag = 1;
	return cmd;
}

navcmd Bebop::rightCMD(unsigned char val){
	navcmd cmd;
	cmd.roll = validatePercent(val);
	cmd.flag = 1;
	return cmd;
}

navcmd Bebop::leftCMD(unsigned char val){
	navcmd cmd;
	cmd.roll = validatePercent(val)*(-1);
	cmd.flag = 1;
	return cmd;
}

/* delta angle or speed ? */
navcmd Bebop::clockwiseCMD(unsigned char val)
{
	navcmd cmd;
	cmd.yaw = validatePercent(val);
	//navcmd.flag = 1; 0x00 or 0x01???
	return cmd;
}

navcmd Bebop::counterClockwiseCMD(unsigned char val)
{
	navcmd cmd;
	cmd.yaw = validatePercent(val)*(-1);
	//navcmd.flag = 1; 0x00 or 0x01???
	return cmd;
}
#endif


// NAV commands => once send it is active for 25 ms 
//                 so send it repeatedly for pitch, roll (in angle)
//                 delta for z
//                 what for yaw ? (angle/sec?) 
//

//-----------------------------------------------------------
// ARCOMMANDS_Generator_GenerateARDrone3PilotingPCMD
//
// uint8 - flag Boolean flag to activate roll/pitch movement
// int8  - roll Roll consign for the drone [-100;100]
// int8  - pitch Pitch consign for the drone [-100;100]
// int8  - yaw Yaw consign for the drone [-100;100]
// int8  - gaz Gaz consign for the drone [-100;100]
// float - psi [NOT USED] - Magnetic north heading of the
//         controlling device (deg) [-180;180]
//----------------------------------------------------------------
int  Bebop::setPCMD(int roll, int pitch, int yawSpeed, int zSpeed)
{
	// check range 
	if(!inRange(-100, roll, 100) ||
	   !inRange(-100, pitch, 100) ||
	   !inRange(-100, yawSpeed, 100) ||
	   !inRange(-100, zSpeed, 100) )
		return -1;
		
	pcmd.roll = pitch;
	pcmd.pitch = pitch;
	pcmd.yawSpeed = yawSpeed ;
	pcmd.zSpeed =  zSpeed;

	return 0;
}

//=======================================================================
//  build PCMD control  packet 
//=======================================================================
void Bebop::buildPCMD(navcmd nav, unsigned char size, unsigned char *returnBuf)
{
    frame_t *pframe = (frame_t *)returnBuf;
    // basic header
    pframe->type = ARConstant::ARNETWORKAL_FRAME_TYPE_DATA;
    pframe->id = ARConstant::BD_NET_CD_NONACK_ID;
    pframe->seq = seq[ARConstant::BD_NET_CD_NONACK_ID]++;
    pframe->size = size;

    // cmd header
    cmd_t *pcmd = (cmd_t *)pframe->data;
    pcmd->project = ARConstant::ARCOMMANDS_ID_PROJECT_ARDRONE3;
    pcmd->clazz = ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTING;
    pcmd->cmd = ARConstant::ARCOMMANDS_ID_ARDRONE3_PILOTING_CMD_PCMD;

    // navigation  
    pcmd->params[0] = nav.flag; // **** 
    pcmd->params[1] = nav.roll;
    pcmd->params[2] = nav.pitch;
    pcmd->params[3] = nav.yawSpeed;
    pcmd->params[4] = nav.zSpeed;

    memcpy(&pcmd->params[5],(void *)&nav.psi, sizeof(float)); // 4 bytes
    return;
}

//==================================================================//
// RX 
//==================================================================//

float Bebop::readFloat(unsigned char *buf)
{
	float returnValue = -1;
	memcpy(&returnValue, buf, 4);
	return returnValue;
}

double Bebop::readDouble(unsigned char *buf)
{
	double returnValue = -1;
	memcpy(&returnValue, buf, 8);
	return returnValue;
}

//=====================================================================
// send to drone 
// mutext used due to control thread and application threads can send 
// TODO: how to detect if wifi connection loss? 
//
//=====================================================================
//ssize_t Bebop::rio_writen(int fd, void *usrbuf, size_t n)
ssize_t Bebop::send2Drone(unsigned char *bufp, size_t n)
{
    size_t  r;
/*
	printf("TX:");
	for(int i = 0 ; i < n;  i++)
		printf("%0X ", bufp[i]);
	printf("\n");
*/
  pthread_mutex_lock(&c2dMutex);
	r = write(c2dSock, bufp, n); 
  pthread_mutex_unlock(&c2dMutex);

	return r;
}

int Bebop::readData(int fd, void *usrbuf, size_t n)
{
    int nleft = n;
    int nread;
    unsigned char *bufp = (unsigned char *)usrbuf;

	int len = 0;
	bool bHasLen = false;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
//	    if (errno == EINTR) /* interrupted by sig handler return */
//			nread = 0;      /* and call read() again */
//	    else
			return -1;      /* errno set by read() */
	}
	else if (nread == 0)
	    break;              /* EOF */

	nleft -= nread;

	if(n - nleft >= 7 && !bHasLen)
	{
		len = *(bufp +3) | (*(bufp +4) << 8) | (*(bufp +5) << 16) | (*(bufp +6) << 24) ;
		nleft += len;
		nleft -= n;
		bHasLen = true;
	}

	bufp += nread;
    }
    return len;         /* return >= 0 */
}
/* Event message : What should I do with this? */

void Bebop::onD2CEventFrame(unsigned char *buf, int length)
{
	unsigned char commandProject = *buf;
	unsigned char commandClass = *(buf+1);
	unsigned short commandId = *(buf+2);
	PILOTING_STATE state = (PILOTING_STATE)commandId;

	if(commandProject == 0x01){

              if(commandClass == 22){
                  std::cout << "mediaStreamingState " << commandId  << std::endl;
               }

		if (commandClass == 0x04){//KH. class=4?   ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTINGSTATE = 4
			//std::cout << "Pilot state" << std::endl;
			switch(state){
                
			case FlyingStateChanged:
				//Read drone's state float
				switch(*(buf+4)){
                       		case LANDED:
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:Landed :" <<  droneState << std::endl;
					break;
                       		case TAKINGOFF:
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:TakingOff :" <<  droneState << std::endl;
					break;
                        	case HOVERING: 
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:Hovering :" <<  droneState << std::endl;
					break;
				case FLYING:
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:Flying :" <<  droneState << std::endl;
					break;
                       		case LANDING:
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:Landing :" <<  droneState << std::endl;
					break;
				case EMERGENCY:
					droneState = (DRONE_STATE)*(buf+4);
					std::cout << "FlyingState:Emergency :" <<  droneState << std::endl;
					break;
				default:
					std::cout << "undefined FlyingState :" << *(buf +4) << std::endl;
				}
				logFlyingStatus((int)*(buf+4));
				break;
                
			 default:
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown comand: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
				break;
			}
		}
		else if(commandClass == 22)
		{
			std::cout << "mediaStreamingState " << commandId  << std::endl; 
			logMediaStatus(commandId);
		}else{
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown class: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
        	}
	}else{
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown pjt: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
    }
}
void Bebop::onD2CNavFrame(unsigned char *buf, int length)
{
	unsigned char commandProject = *buf;
	unsigned char commandClass = *(buf+1);
	unsigned short commandId = *(buf+2);

	PILOTING_STATE state = (PILOTING_STATE)commandId;

	if(commandProject == 0x01){
              if(commandClass == 22){
                  std::cout << "mediaStreamingState " << commandId  << std::endl;
               }
		if (commandClass == 0x04){//KH. class=4?   ARConstant::ARCOMMANDS_ID_ARDRONE3_CLASS_PILOTINGSTATE = 4
			switch(state){
                
            // how accurate it is? GPS or others..    
			case PositionStateChanged:
				//Read latitude (double) longitude (double) altitude (double)
				latitude = readDouble(buf+4);
				longitude = readDouble(buf+12);
				altitude = readDouble(buf+20);
				//std::cout << "Latitude: " << latitude << std::endl;
				//std::cout << "Longitude: " << longitude << std::endl;

				//Don't write default value;
				logGPSInfo(latitude, longitude, altitude);

				//Write coordinate into file
				//Read value when latitude, longitude >= 0
				break;
            
            // accurate            
			case SpeedChanged:
				//Read SpeedX (float) SpeedY(float) SpeedZ(float)
				speedX = readFloat(buf+4);
				speedY = readFloat(buf+8);
				speedZ = readFloat(buf+12);
			  logSpeedInfo(speedX, speedY, speedZ);
				//std::cout << "Speed:" << speedX << "," << speedY << "," << speedZ << std::endl;
				break;

            // attitude    
			case AttitudeChanged:
				//Read Roll (float) Pitch (float) Yaw (float)
				roll = readFloat(buf+4);
				pitch = readFloat(buf+8);
				yaw = readFloat(buf+12);
			  logAttitudeInfo(roll, pitch, yaw);
				//std::cout << "Roll: " << roll << std::endl;
				//std::cout << "Pitch: " << pitch << std::endl;
				//std::cout << "Yaw: " << yaw << std::endl;
				break;
                
			case AltitudeChanged:
				//Read Altitude (double)
				altitude = readDouble(buf+4);
				break;

			 default:
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown comand: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
				break;
			}
		}else if(commandClass == 22){
			std::cout << "mediaStreamingState " << commandId  << std::endl; 
			logMediaStatus(commandId);
		}else{
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown class: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
        	}
	}else{
						logUndefinedMsg(commandProject, commandClass, commandId, buf, 0);
/*
            			std::cerr        << "unknown pjt: " 
						 << (int) commandProject << ","
						 << (int) commandClass  << "," 
						 << (int) commandId << std::endl;
*/
    }
}

/* RX Message 
   
   event     :
   navgation :
   ping-pong :
   video     : not used since 4.0
      
*/

void Bebop::onD2CFrame()
{
	unsigned char rxBuff[MAX_FRAME_SIZE];
	//bool isVideoStreaming = false;


    
    // 1. Read data and return len of message:
    // TODO: select and UDP, so simply read it !
	int len = readData(d2cSock, (void *)&rxBuff, MAX_FRAME_SIZE);  // 7 = header size
	//std::cout << "On D2C data received." << len <<  std::endl;
	if(len <= 0) //Can not read data
		return ;
	
	// 2. Copy data to frame variable
	frame_t pframe;
	memcpy(&pframe, &rxBuff, 7);
	memcpy(pframe.data, &rxBuff[7], len - 7);
	pframe.data[len+1] = 0;


        // validation of frame 
        if(pframe.type > 4){
        	std::cerr << "undefined Type:" << (int) pframe.type << std::endl;   
    	}
       
    	// 3. parsing and processing 
	/*
	if (pframe.type == 128
	 && pframe.id == 96) {
		onVideoFrame(&rxBuff[7], len - 7);
	}
	*/
	if (pframe.type == ARConstant::ARNETWORKAL_FRAME_TYPE_DATA_WITH_ACK){
							// 4,
		sendAck(&pframe);
        	//std::cout << "ACK packet: id = " << (int) pframe.id << std::endl;
	}

	/*if(!isVideoStreaming){
		opencv();
		isVideoStreaming = true;
	}*/

	if (pframe.id == ARConstant::BD_NET_DC_EVENT_ID){
        	if(pframe.size > 0)
			onD2CEventFrame(pframe.data, pframe.size - 7); // event changes   
	}else if(pframe.id == ARConstant::BD_NET_DC_NAVDATA_ID){
		if(pframe.size > 0)
			onD2CNavFrame(pframe.data, pframe.size - 7);   // navigation status 
	}else if(pframe.id == ARConstant::ARNETWORK_MANAGER_INTERNAL_BUFFER_ID_PING){
     std::cout << "pinged" << std::endl;

		sendPong(&pframe);
	}else{
        	std::cerr << "unknown id" << (int)pframe.id << std::endl; 
    	}

}



//=========================================================================
// start drone 
//
// siscovery; create udp socks; start thread
//=========================================================================
int Bebop::start()
{
	int result;
	
	struct stat st = {0};
	// to class
	const char* destIP = "192.168.42.1";
	//TODO : erase controlerIP if don't need it
	//const char* controlerIP = "192.168.42.2";

  // 1. discovery 
	if(discoverDrone(destIP) < 0){
		std::cerr << "discovery failed" << std::endl;
        	return -1;  
    	}else{
		std::cout << "discovery success" << std::endl;
    	}
    
	// 2. make UDP connections (Up, Down)
	d2cSock = setupd2cSocket(43210); // my port
	c2dSock = setupc2dSocket(destIP, 54321); // drone ip, port

	// 3. run the drone agent thread
	return pthread_create(&tid, NULL, run, this);

}


//==========================================================================
// timed Rx for drone repsonse 
//
// @TODO: handle  send-command event pipe
//========================================================================== 
int Bebop::waitControlEvent(int delayInms = 10)
{
        char ckey;
        fd_set s_rd;
        struct timeval delay = {0L, delayInms*1000L};
        int fd = d2cSock;

        FD_ZERO(&s_rd);
        FD_SET(fd, &s_rd);
        int n = select(fd+1, &s_rd, NULL, NULL, &delay);
        if(n < 0){
                std::cerr << "error in select" << std::endl;
		return -2;
        }else if(n > 0){
                if(FD_ISSET(fd, &s_rd)){
        		onD2CFrame();
                }else{
                        std::cerr << "select from unknow fd" << std::endl;
                        return -3;
                }
        } // else timeout

        return 0;
}

//=======================================================================
//  the controller thread 
//
//  mainly waiting and processing  message from drone
//  
//=======================================================================
void *Bebop::innerRun()
{

	// init the drone
	findAllState();

	flattrim();

	while(!stopControlReq)
	{
		// wait for RX
                waitControlEvent(25); // 25 ms
	
		// TX periodic  
		if(droneState == FLYING || droneState == HOVERING ){  
			sendPCMD();
		}
	}
	
	return NULL;
}


