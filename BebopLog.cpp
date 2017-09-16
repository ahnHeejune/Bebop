//============================================================================
// Log 
//  - log file create 
//  - logging 
//  - log file close (not yet) 
//  - mutex for multi-thread access (not yet)
//
//=====================================================================================
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "Bebop.h"

/* static constants */
const char* Bebop::LOG_DIR_PATH = (char *)"Log";
const char* Bebop::LOG_FILE_PREFIX = (char *)"Log/drone";

//=============================================================================
// Log file create 
//=============================================================================
int Bebop::logInit()
{
  char filename[1024]; 
	char strdate[256];
	struct stat st = {0};
	struct tm *t;

  pthread_mutex_init(&logMutex, NULL); // logMutex = PTHREAD_MUTEX_INITIALIZER; 

  // 1. log directory  
	if (stat(LOG_DIR_PATH, &st) == -1) // not exist
		mkdir(LOG_DIR_PATH, 0700);


	// 2. log file name with time info
	time_t now = time(NULL);
	t = gmtime(&now);
        strftime(strdate, sizeof(strdate),"-%Y-%m_%d_%H:%M:%S.log", t); 
        strcpy(filename, LOG_FILE_PREFIX);
        strcat(filename, strdate);
	
        // 3. open file to write
	logFptr = fopen(filename, "w");  
	if(logFptr == NULL){
		std::cout << "Can not open file" << std::endl;
		return -1;
	}

        return 0; // all ok
}

//====================================================================
// close all resources 
// mutex lock, log file  
//====================================================================
int Bebop::logClose()
{
    int status = pthread_mutex_destroy(&logMutex); 
    if(status < 0)
			std::cerr << "error in destroying log mutex" << std::endl;

		if(logFptr) 
			status = fclose(logFptr);

	  return status;
}


//=============================================================================
//
//  elpased time in ms
//=============================================================================
unsigned long Bebop::elapsedTime( )
{
        //timeval* curr = NULL;
        timeval curr;
        gettimeofday(&curr, NULL);

        return (curr.tv_sec*1000 + curr.tv_usec/1000 - startTime.tv_sec*1000 - startTime.tv_usec/1000);
}


//===========================================================================
// Drone flying status  message 
// LANDED => TAKINGOFF => HOVERING => FLYING => LANDING => LANDED 
//=============================================================================
bool Bebop::logFlyingStatus(int status)  
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "fst %ld %d\n", elapsedTime(), status);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//===========================================================================
// Sending Drone flying State command 
//=============================================================================
bool Bebop::logSetFly(int mode)  
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "setfly %ld %d\n", elapsedTime(), mode);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}
//===========================================================================
// Set media status  
//=============================================================================
bool Bebop::logSetPCMD(int pitch, int roll, int yawSpeed, int gaz)  
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "pcmd %ld %d %d %d %d \n", elapsedTime(), pitch, roll, yawSpeed, gaz);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}
//===========================================================================
// Set media status  
//=============================================================================
bool Bebop::logSetMedia(int status)  
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "med %ld %d\n", elapsedTime(), status);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}
//===========================================================================
// media status report 
//=============================================================================
bool Bebop::logMediaStatus(int status)  
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "med %ld %d\n", elapsedTime(), status);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//=============================================================================
// Log GPS 
// gps <lat>  <lon>
//=============================================================================
bool Bebop::logGPSInfo(double lat, double lon, double alt)
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "gps %ld  %.6lf %.6lf %.6lf \n", elapsedTime(), lat, lon, alt);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//===========================================================================
// Drone speed mesaage 
// spd time  vx, vy, vz 
//=============================================================================
bool Bebop::logSpeedInfo(float speedx, float speedy, float speedz)
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "spd %ld %.3f %.3f %.3f\n", elapsedTime(), speedx, speedy, speedz); // bug fixed 2017. 09.16
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//===========================================================================
// Drone attitude mesaage 
// roll, pitch, yawspeed, gaz in RADIAN
//=============================================================================
bool Bebop::logAttitudeInfo(float pitch, float roll, float yaw )
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "att %ld %.3f %.3f %.3f\n", elapsedTime(), roll, pitch, yaw);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//===========================================================================
// Drone altitude message 
//=============================================================================
bool Bebop::logAltitudeInfo(double alt)
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "alt %ld %.3lf\n", elapsedTime(), alt);
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}

//===========================================================================
// undefined commands log 
// log rxed the undefined command 
// mainly log the pjt, class, and cmd id, optioally print the data 
//=============================================================================
bool Bebop::logUndefinedMsg(int pjt, int clazz, int cmd, unsigned char *msg, int printlen)
{
	if(logFptr == NULL){
		std::cout << "no file opened" << std::endl;
		return false;
	}

  pthread_mutex_lock(&logMutex);   
	fprintf(logFptr, "udef %ld %d %d %d:", elapsedTime(), pjt, clazz, cmd);
  if(!msg && printlen > 0){
		for(int i = 0; i < printlen; i++)
			fprintf(logFptr, "%0X ", msg[i]);
	}
	fprintf(logFptr, "\n" );
	fflush(logFptr);
  pthread_mutex_unlock(&logMutex);   
	return true;
}




