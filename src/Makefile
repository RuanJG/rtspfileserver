TOP_DIR := $(shell pwd)


#x86
#CROSS_COMPILE=
#DEP_LIBS = -L$(TOP_DIR)/lib/x86/lib
#HEADER = -I$(TOP_DIR)/lib/x86/include

#mips
CROSS_COMPILE= mipsel-openwrt-linux-uclibc-
#DEP_LIBS = -L$(TOP_DIR)/lib/mips/lib
#HEADER = -I$(TOP_DIR)/lib/mips/include


X264_LIB= $(TOP_DIR)/libx264/src

#make
C_COMPILER =    $(CROSS_COMPILE)gcc
AR_COMPILER =    $(CROSS_COMPILE)ar
CPLUSPLUS_COMPILER =    $(CROSS_COMPILE)g++
LIBS    = -lpthread -lx264 -lm -ldl

rtspfileserver:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h config.h video_capture.c video_capture.h h264encoder.h
	#$(MAKE) -C $(X264_LIB)
	$(CPLUSPLUS_COMPILER) -o $@ $?  -L. $(LIBS)



clean:
	rm rtspfileserver *.o
