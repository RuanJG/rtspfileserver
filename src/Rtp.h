#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"

#ifndef _RTP_H
 #define _RTP_H
	/*  PT       encoding    media type
		96-127    dynamic         ?     */
	#define PayloadType 96
	#define fps 30
	#define sleepTime 15000 
	extern bool lock;
	 struct RtpData{
		sockaddr_in addrClient;
		int rtpServerPort;
		int rtpClientPort;
	};
	extern struct RtpData RtpParameter;
	void *Rtp(void *fileName);
	void *Rtp_camera(void *came);
void * rtp_worker(void *came);
void * camera_worker(void *came);
	void createRtpSocket(int *serverFD,sockaddr_in *addrClient);
	int OpenVideoFile(char *fileName);
	void createRtpHeader();
	void setRtpVersion();
	void setRtpPayloadType();
	void setSequenceNumber(int SequenceNumber);
	void setTimestamp(unsigned int timestamp);
	void setSSRC(unsigned int ssrc);
	void setMarker(int marker);
	int RtpEncoder(int sockFD,struct sockaddr_in addrClient,char *FrameStartCode,int FrameLength,int *SequenceNumber,unsigned int *timestamp);
	void setFUIndicator(char *FrameStartIndex);
	void setFUHeader(char *FrameStartIndex,bool start,bool end);
	static int RtpJpegEncoder(int sockfd, struct sockaddr_in addrClient,unsigned char *in,int outsize,int width,int height);
	static unsigned int convertToRTPTimestamp(/*struct timeval tv*/);


#endif
