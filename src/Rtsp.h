#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <signal.h>

#ifndef _RTSP_H_
 #define _RTSP_H

	#define BUF_SIZE 1024
	#define RtpServerPort 50000
	void Rtsp(int args,char *argv[]);
	void OPTIONS_Reply(int clientFD);
	void DESCRIBE_Reply(int clientFD,char *RtspContentBase);
	void SETUP_Reply(int clientFD);
	void PLAY_Reply(int clientFD,struct sockaddr_in addrClient,char *RtspUrl,char *fileName);
	void GET_PARAMETER_Reply(int clientFD);
	void TEARDOWN_Reply(int clientFD);
	void createRtspSocket(int *serverFD,int *clientFD,struct sockaddr_in *addrClient);
void do_release_thread();
void quit_handler( int sig );
#endif
