#include "FFRtpVidClient.h"

RtpVidClient *RtpVidClient::getInstance()
{
   // now only one type
   return new FFRtpVidClient();
}




