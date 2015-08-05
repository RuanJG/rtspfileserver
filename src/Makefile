# Which compiler
CC = g++
#Where the library
LIBS    = -lpthread

RtspServer:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h config.h video_capture.c video_capture.h
	$(CC) -o $@ $? $(LIBS) 

clean:
	rm RtspServer
