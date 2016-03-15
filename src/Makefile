TOP_DIR := $(shell pwd)


#x86
CROSS_COMPILE=
#DEP_LIBS = -L$(TOP_DIR)/lib/x86/lib
#HEADER = -I$(TOP_DIR)/lib/x86/include

#mips
#CROSS_COMPILE= mipsel-openwrt-linux-uclibc-
#DEP_LIBS = -L$(TOP_DIR)/lib/mips/lib
#HEADER = -I$(TOP_DIR)/lib/mips/include


#make
C_COMPILER =    $(CROSS_COMPILE)gcc
AR_COMPILER =    $(CROSS_COMPILE)ar
CPLUSPLUS_COMPILER =    $(CROSS_COMPILE)g++
LIBS    = -lpthread -lx264 -lm -ldl -lcamkit

rtspfileserver:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h config.h video_capture.c video_capture.h h264encoder.h
	$(CPLUSPLUS_COMPILER) -o $@ $?  -L. $(LIBS) -I camkit_h

install:
	sudo cp ../files/rtspfileserver.sh /bin/rtspfileserver.sh
	sudo cp ../files/rtspfileserverd /etc/init.d/rtspfileserverd
	sudo cp ./libcamkit.so /usr/lib/libcamkit.so
	sudo cp ./rtspfileserver  /bin/rtspfileserver
	sudo chmod 0777 /bin/rtspfileserver
	sudo chmod 0777 /bin/rtspfileserver.sh
	sudo update-rc.d rtspfileserverd  defaults
	sudo ldconfig
uninstall:
	sudo update-rc.d -f rtspfileserverd remove

clean:
	rm rtspfileserver *.o
