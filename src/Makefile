TOP_DIR := $(shell pwd)
#CROSS_COMPILE=        mipsel-openwrt-linux-uclibc-
CPLUSPLUS_COMPILER =    $(CROSS_COMPILE)g++
C_COMPILER =    $(CROSS_COMPILE)gcc
DEP_LIBS = -L$(TOP_DIR)/lib/x86/lib
HEADER = -I$(TOP_DIR)/lib/x86/include
#Where the library
LIBS    = -lpthread -lx264 -lm -ldl

rtspfileserver:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h config.h video_capture.c video_capture.h h264encoder.h h264encoder.c
	$(CPLUSPLUS_COMPILER) -o $@ $? $(LIBS) $(HEADER) $(DEP_LIBS)

clean:
	rm rtspfileserver
