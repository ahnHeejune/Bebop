CPP := g++
CPPFLAGS = -O2 -g
LDFLAGS = -lpthread -lm -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio 

OBJS =  BebopApp.o BebopControl.o BebopLog.o BebopVid.o ARConstant.o 

TARGET = bebopApp 

all: $(TARGET) 

$(TARGET):$(OBJS) 
	$(CPP) $(OBJS) $(LDFLAGS) -o $@

%.o:%.cpp Bebop.h
	$(CPP) $(CPPFLAGS) -c $<


clean:
	rm -rf *.o $(TARGET)
 
