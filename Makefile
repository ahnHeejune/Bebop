CPP := g++
CPPFLAGS = -O2 -g
LDFLAGS = -lpthread -lm -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio 
LDFLAGS += -lavcodec -lavformat -lavutil -lswscale  # for ffmpeg

OBJS =  BebopApp.o BebopControl.o BebopLog.o BebopVid.o ARConstant.o RtpVidClient.o  FFRtpVidClient.o PID.o 
#BebopVision.o 

TARGET = bebopApp 

all: $(TARGET) 

$(TARGET):$(OBJS) 
	$(CPP) $(OBJS) $(LDFLAGS) -o $@

#%.o:%.cpp Bebop.h PID.h BebopVision.h RtpVidClient.h FFRtpVidClient.h
%.o:%.cpp Bebop.h PID.h RtpVidClient.h FFRtpVidClient.h
	$(CPP) $(CPPFLAGS) -c $<


clean:
	rm -rf *.o $(TARGET)
 
