# Which compiler
CROSS_COMPILE=        mipsel-openwrt-linux-uclibc-
CPLUSPLUS_COMPILER =    $(CROSS_COMPILE)g++
#Where the library
LIBS    = -lpthread

rtspfileserver:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h config.h video_capture.c video_capture.h
	$(CPLUSPLUS_COMPILER) -o $@ $? $(LIBS) 

clean:
	rm rtspfileserver
