#include "Rtsp.h"
#include "Rtp.h"
#include "config.h"
#include "video_capture.h"

using namespace std;

//RTSP¶Ç¿é©Ò»ÝÀÉ®×
char *RtspResponse = "RTSP/1.0 200 OK\r\n";
char *RtspServer = "Server: VLC\r\n";
char *RtspCachControl = "Cache-Control: no-cache\r\n";
char *RtspSession = "Session: ee62ba70a1ddca;timeout=60\r\n";
string RtspContentLength = "Content-Length: \r\n";
string RtspCseq = "Cseq: \r\n";
string RtpClientPort;
int RtspCseqNumber = 2;
//RTP Thread use
bool lock;
struct RtpData RtpParameter;
//RTSPªº¶Ç¦¬BUFFER
char recvBuf[BUF_SIZE];
char sendBuf[BUF_SIZE];
struct camera cam;
bool use_camera;

pthread_t rtp_tid=0;
pthread_t cam_tid=0;
int serverFD,clientFD;//Server and Client SocketID

int fps_time_us = 1000000/FPS;

void Rtsp(int args,char *argv[])
{
	//¦s©ñOPTIONS DESCRIBE SETUP PLAY TEARDOWNµ¥
	char *RequestType;
	char *RtspUrl;
	//TCP³s½u©Ò»Ý
	int serverFD,clientFD;//Server and Client SocketID
	struct sockaddr_in addrClient;
	socklen_t addrClientLen = sizeof(addrClient);
	int retVal;

	char * fileName = NULL;

	if( args == 2 ){ 
		use_camera = 0;
		fileName = argv[args-1];
		log_msg("fileName=%s\n",fileName);	
	}
	if( args == 3 ){
		use_camera = 1;
		cam.width = atoi(argv[1]);
		cam.height = atoi(argv[2]);
		log_msg("camera use pix= %dX%d\n",cam.width,cam.height);
	}

    if( cam.width == 0 || cam.height == 0){
	    //set it by args
    	cam.width         = CAM_WIDGH;
    	cam.height        = CAM_HEIGHT;
	log_msg("camera use %dX%d\n",cam.width,cam.height);
    }


#ifdef USE_CAMERA_THREAD
 	//if( 0 >  cam.camBuff.init(cam.width*2*cam.height)){
 	if( 0 >  cam.camBuff.init(cam.width*2*cam.height)){
		log_msg("cameraBuffer init failed\n");
	}
#endif	

	signal(SIGINT,quit_handler);


	//RtspSocket
	serverFD = 0;
	clientFD = 0;
	createRtspSocket(&serverFD,&clientFD,&addrClient);

	while(1)
	{
		retVal = recv(clientFD,recvBuf,BUF_SIZE,0);
		if(retVal == -1 || retVal == 0){
			close(clientFD);//Ãö³¬Socket
			clientFD = 0;
			log_msg("Server is listening.....\n");
			clientFD = accept(serverFD,(struct sockaddr*)&addrClient,&addrClientLen);
			if(clientFD == -1){
				perror("Accept failed!!\n");
				close(serverFD);//Ãö³¬Socket
				serverFD = 0;
				break;
			}else log_msg("succeed!!\n");
			RtspCseqNumber = 2;
			continue;
		}
		RequestType = (char*) malloc(strlen(recvBuf)+1);
		strcpy(RequestType,recvBuf);
		strtok(RequestType," ");
		RtspUrl = strtok(NULL," ");
		//log_msg("RequestType=%s,RtspUrl=%s\n",RequestType,RtspUrl);
		//log_msg("RecvBuf :\n%s",recvBuf);
		if(!strcmp(RequestType,"OPTIONS"))
			OPTIONS_Reply(clientFD);
		else if(!strcmp(RequestType,"DESCRIBE"))
			DESCRIBE_Reply(clientFD,RtspUrl);
		else if(!strcmp(RequestType,"SETUP"))
			SETUP_Reply(clientFD);
		else if(!strcmp(RequestType,"PLAY"))
			PLAY_Reply(clientFD,addrClient,RtspUrl,fileName);
		else if(!strcmp(RequestType,"GET_PARAMETER"))
			GET_PARAMETER_Reply(clientFD);
		else if(!strcmp(RequestType,"TEARDOWN"))
			TEARDOWN_Reply(clientFD);
		else
		{
			close(clientFD);//Ãö³¬Socket
			clientFD = 0;
			log_msg("Server is listening.....\n");
			clientFD = accept(serverFD,(struct sockaddr*)&addrClient,&addrClientLen);
			if(clientFD == -1){
				perror("Accept failed!!\n");
				close(serverFD);//Ãö³¬Socket
				serverFD = 0;
				break;
			}else log_msg("succeed!!\n");
			RtspCseqNumber = 2;
			continue;
		}
		RtspCseqNumber++;
		free(RequestType);	
	}
#ifdef USE_CAMERA_THREAD
    cam.camBuff.deinit();
#endif
	system("pause");
	exit(1);

}

void createRtspSocket(int *serverFD,int *clientFD,sockaddr_in *addrClient)
{
	struct sockaddr_in addrServer;
	socklen_t addrClientLen = sizeof(*addrClient);

	//«Ø¥ßSocket
	*serverFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	//Server Socket IP
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(RtspServerPort);

	//¸j©wSocket
	if(bind(*serverFD,(sockaddr*)&addrServer,sizeof(addrServer)) == -1){
		perror("Bind failed!\n");
		close(*serverFD);//Ãö³¬Socket
		*serverFD = 0;
		exit(0);
	}
	//«Ø¥ßºÊÅ¥
	if(listen(*serverFD,10) == -1){
		perror("Listen failed!!\n");
		close(*serverFD);//Ãö³¬Socket
		*serverFD = 0;
		exit(0);
	}
	//±µ¦¬«È¤áºÝ½Ð¨D
	log_msg("Server is listening %d .....",RtspServerPort);
	*clientFD = accept(*serverFD,(sockaddr*)addrClient,&addrClientLen);
	if(*clientFD == -1){
		perror("Accept failed!!\n");
		close(*serverFD);//Ãö³¬Socket
		*serverFD = 0;
		*clientFD = 0;
		exit(0);
	}
	else log_msg("succeed!!\n");
}
char *int2str(int i)
{
	char *s;
	s = (char *)malloc(i);
	sprintf(s,"%d",i);
	return s;
}
int str2int(string temp)
{
	return atoi(temp.c_str());
}
string getRtpClientPort()
{
	string temp;
	string retVal;
	int begin,end;

	log_msg("\n******getRtpClientPort******\n");
	temp.assign(recvBuf);
	begin = temp.find("client_port=")+12;
	end = begin + 11;

	retVal.append(temp.begin()+begin,temp.begin()+end);
	log_msg("retVal=%s\n",retVal.c_str());

	return retVal;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <camkit.h>
#ifdef __cplusplus
}
#endif

#define MAX_RTP_SIZE 1420
#define WIDTH 800
#define HEIGHT 600
#define FRAMERATE 15

unsigned long get_current_ms()
{
	struct timeval t_start;
	gettimeofday(&t_start, NULL); 
	unsigned long start = ((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000;
	return start;
}
int get_s()
{
	struct timeval t_start;
	gettimeofday(&t_start, NULL); 
	return t_start.tv_sec;
}
unsigned long last_ms=0;
int simple_demo(char * ip, int port)
{
	/*
    if (argc < 3)
    {
        printf("Usage: #simple_demo ip port\n");
        return  -1;
    }
    */

	struct cap_handle *caphandle = NULL;
	struct cvt_handle *cvthandle = NULL;
	struct enc_handle *enchandle = NULL;
	struct pac_handle *pachandle = NULL;
    struct net_handle *nethandle = NULL;
	struct cap_param capp;
	struct cvt_param cvtp;
	struct enc_param encp;
	struct pac_param pacp;
    struct net_param netp;

	// set paraments
	U32 vfmt = V4L2_PIX_FMT_YUYV;
	U32 ofmt = V4L2_PIX_FMT_YUV420;

	capp.dev_name = "/dev/video0";
	capp.width = WIDTH;
	capp.height = HEIGHT;
	capp.pixfmt = vfmt;
	capp.rate = FRAMERATE;

	cvtp.inwidth = WIDTH;
	cvtp.inheight = HEIGHT;
	cvtp.inpixfmt = vfmt;
	cvtp.outwidth = WIDTH;
	cvtp.outheight = HEIGHT;
	cvtp.outpixfmt = ofmt;

	encp.src_picwidth = WIDTH;
	encp.src_picheight = HEIGHT;
	encp.enc_picwidth = WIDTH;
	encp.enc_picheight = HEIGHT;
	encp.chroma_interleave = 0;
	encp.fps = FRAMERATE;
	encp.gop = 12;//30;
	encp.bitrate = 1000;//800;

	pacp.max_pkt_len = MAX_RTP_SIZE - 20;
	pacp.ssrc = 10;

    netp.type = UDP;
    //netp.serip = argv[1];
    //netp.serport = atoi(argv[2]);
    netp.serip = ip;
    netp.serport = port;

	caphandle = capture_open(capp);
	if (!caphandle)
	{
		log_msg("--- Open capture failed\n");
		return -1;
	}

	cvthandle = convert_open(cvtp);
	if (!cvthandle)
	{
		log_msg("--- Open convert failed\n");
		return -1;
	}

	enchandle = encode_open(encp);
	if (!enchandle)
	{
		log_msg("--- Open encode failed\n");
		return -1;
	}

	pachandle = pack_open(pacp);
	if (!pachandle)
	{
		log_msg("--- Open pack failed\n");
		return -1;
	}

    nethandle = net_open(netp);
    if (!nethandle)
    {
        log_msg("--- Open network failed\n");
        return -1;
    }

	// start stream loop
	int ret;
	void *cap_buf, *cvt_buf, *hd_buf, *enc_buf;
	char *pac_buf = (char *) malloc(MAX_RTP_SIZE);
	int cap_len, cvt_len, hd_len, enc_len, pac_len;
	enum pic_t ptype;
	unsigned long framecount = 0;

	capture_start(caphandle);		// !!! need to start capture stream!

	unsigned long now_ms;
	int total_len = 0;

	while (1)
	{
		now_ms = get_current_ms();
		if( (now_ms - last_ms)>1000) {
			log_msg("--last=%d,now=%d \n",last_ms,now_ms);
			log_msg("------------ current frame count=%d,len=%d\n",framecount,total_len );
			framecount = 0;
			last_ms = now_ms;
		}
		ret = capture_get_data(caphandle, &cap_buf, &cap_len);
		if (ret != 0)
		{
			if (ret < 0)		// error
			{
				log_msg("--- capture_get_data failed\n");
				break;
			}
			else	// again
			{
				log_msg("recheck camera data\r\n");
				usleep(10000);
				continue;
			}
		}
		if (cap_len <= 0)
		{
			log_msg("!!! No capture data\n");
			continue;
		}
		log_msg("--get pic len = %d\n",cap_len);
		// else

		ret = convert_do(cvthandle, cap_buf, cap_len, &cvt_buf, &cvt_len);
		if (ret < 0)
		{
			log_msg("--- convert_do failed\n");
			break;
		}
		if (cvt_len <= 100)
		{
			log_msg("!!! No convert data\n");
			continue;
		}
		log_msg("cvt_len=%d \n",cvt_len);
		// else

		// fetch h264 headers first!
		while ((ret = encode_get_headers(enchandle, &hd_buf, &hd_len, &ptype))
				== 1)
		{
            //fwrite(hd_buf, 1, hd_len, dumpfile);
			pack_put(pachandle, hd_buf, hd_len);
			while (pack_get(pachandle, pac_buf, MAX_RTP_SIZE, &pac_len) == 1)
			{
                ret = net_send(nethandle, pac_buf, pac_len);
				if (ret != pac_len)
				{
					log_msg("send pack data failed, size: %d, err: %s\n", pac_len,
							strerror(errno));
				}
			}
		}

		ret = encode_do(enchandle, cvt_buf, cvt_len, &enc_buf, &enc_len,
				&ptype);
		if (ret < 0)
		{
			log_msg("--- encode_do failed\n");
			break;
		}
		if (enc_len <= 100)
		{
			log_msg("!!! No encode data\n");
			continue;
		}
		// else

		log_msg("-- send packet %d B\n",enc_len);
		if( enc_len < 100 ) continue;
        //fwrite(enc_buf, 1, enc_len, dumpfile);
		// RTP pack and send
		pack_put(pachandle, enc_buf, enc_len);
		while (pack_get(pachandle, pac_buf, MAX_RTP_SIZE, &pac_len) == 1)
		{
            ret = net_send(nethandle, pac_buf, pac_len);
			if (ret != pac_len)
			{
				log_msg("send pack failed, size: %d, err: %s\n", pac_len,
						strerror(errno));
			}
		}

		framecount++;
		total_len+=enc_len;
	}
	capture_stop(caphandle);

	free(pac_buf);
    net_close(nethandle);
	pack_close(pachandle);
	encode_close(enchandle);
	convert_close(cvthandle);
	capture_close(caphandle);

	return 0;
}

void *test_Rtp_camera(void *came)
{
	log_msg("start simple_demo :%s:%d\n",(char*)inet_ntoa(RtpParameter.addrClient.sin_addr),RtpParameter.rtpClientPort);
	simple_demo((char*)inet_ntoa(RtpParameter.addrClient.sin_addr),RtpParameter.rtpClientPort );
}

void createRtpThread(char* fileName)
{
	int res;
	pthread_t tid;
	void *thread_result;

	//cam.camBuff.status = -1;
	if( use_camera ){
#ifdef USE_CAMERA_THREAD
		res = pthread_create(&cam_tid,NULL,camera_worker,(void*)&cam);
		res = pthread_create(&rtp_tid,NULL,rtp_worker,(void*)&cam);
#else
		res = pthread_create(&tid,NULL,test_Rtp_camera,(void*)&cam);
#endif
	}else
		res = pthread_create(&tid,NULL,Rtp,(void*)fileName);

	if(res!=0){
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
}
void OPTIONS_Reply(int clientFD)
{
	char *RtspPublic = "Public: DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,GET_PARAMETER\r\n\r\n";
	string temp;

	log_msg("\n*****OPTIONS*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),RtspPublic);
	free(int2str(RtspCseqNumber));

	//log_msg("Send Temp:\n%s",temp.c_str());

	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed1!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);

}
void DESCRIBE_Reply(int clientFD,char *RtspContentBase)
{
	char *RtspContentType = "Content-type: application/sdp\r\n";

char *SDPFile = "m=video 2000 RTP/AVP 96\r\na=rtpmap:96 H264\r\na=framerate:15\r\nc=IN IP4 0.0.0.0\r\n";
//char *SDPFile = "m=video 2000 RTP/AVP 26\r\na=rtpmap:26 JPEG\r\na=framerate:15\r\nc=IN IP4 0.0.0.0\r\n";

	string temp;

	log_msg("\n*****DESCRIBE*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspContentType);
	temp.insert(strlen(temp.c_str()),"Content-Base: ");
	temp.insert(strlen(temp.c_str()),RtspContentBase);
	temp.insert(strlen(temp.c_str()),"\r\n");
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,int2str(strlen(SDPFile)));
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	temp.insert(strlen(temp.c_str()),SDPFile);
	free(int2str(strlen(SDPFile)));
	free(int2str(RtspCseqNumber));

	//log_msg("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed2!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);
}
void SETUP_Reply(int clientFD)
{
	char *RtspTransport = "Transport: RTP/AVP/UDP;unicast;";
	char *ssrc = ";ssrc=15F6B7CF;";
	char *mode = "mode=play\r\n";
	string RTPServerPort = ";server_port=10000-10001";
	string temp;

	RtpClientPort = getRtpClientPort();
	log_msg("\n*****SETUP*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspTransport);
	temp.insert(strlen(temp.c_str()),"client_port=");
	temp.insert(strlen(temp.c_str()),RtpClientPort);
	temp.insert(strlen(temp.c_str()),RTPServerPort);
	temp.insert(strlen(temp.c_str()),ssrc);
	temp.insert(strlen(temp.c_str()),mode);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,"4");
	temp.insert(strlen(temp.c_str()),"\r\n");

	//log_msg("Send temp:\n%s",temp.c_str());

	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed3!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	//ZeroMemory(sendBuf,BUF_SIZE);
	
}
void PLAY_Reply(int clientFD,struct sockaddr_in addrClient,char *RtspUrl,char *fileName)
{
	char *RTPInfo = "RTP-Info: url=";
	char *seq = ";seq=5873;";
	char *rtptime = "rtptime=2217555422\r\n";
	char *Range = "Range: npt=10-\r\n";
	string temp;
	//RTP©Ò»Ý°Ñ¼Æ
	//struct RtpData *RtpParameter;
	//RtpParameter = (RtpData*)malloc(sizeof(RtpData));

	log_msg("\n*****Play*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RTPInfo);
	temp.insert(strlen(temp.c_str()),RtspUrl);
	temp.insert(strlen(temp.c_str()),seq);
	temp.insert(strlen(temp.c_str()),rtptime);
	temp.insert(strlen(temp.c_str()),Range);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//log_msg("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed4!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	lock = 1;
	bzero(sendBuf,BUF_SIZE);
	//°e§¹PLAY_Reply«á¶}©l¶i¦æRTP¶Ç¿é
	RtpParameter.addrClient = addrClient;
	RtpParameter.rtpServerPort = 10000;
	RtpParameter.rtpClientPort = str2int(RtpClientPort);
	createRtpThread(fileName);
}
void GET_PARAMETER_Reply(int clientFD)
{
	string temp;

	log_msg("\n*****GET_PARAMETER*****\n");

	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//log_msg("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed0000!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);
}
void TEARDOWN_Reply(int clientFD)
{
	string temp;

	log_msg("\n*****TEARDOWN*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//log_msg("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		log_msg("Sent failed5!!\n");
		close(clientFD);//Ãö³¬Socket
		exit(0);
	}
	lock = 0;
#ifdef USE_CAMERA_THREAD
	do_release_thread();
#endif
	bzero(sendBuf,BUF_SIZE);
	bzero(recvBuf,BUF_SIZE);
}

void do_release_thread()
{
	lock = 0;
	if( rtp_tid != 0 )
		pthread_join(rtp_tid,NULL);
	if( cam_tid != 0 )
		pthread_join(cam_tid,NULL);

	rtp_tid = 0;
	cam_tid = 0;
}
void do_release_socket()
{
	if( clientFD != 0 )
		close(clientFD);
	if( serverFD != 0 )
		close(serverFD);
	clientFD = 0;
	serverFD = 0;
}
void quit_handler( int sig )
{
	lock = 0;
	do_release_thread();
	do_release_socket();
#ifdef USE_CAMERA_THREAD
    cam.camBuff.deinit();
#endif	
    	log_msg("Ruan: exit rtspfileserver by kill\n");
	exit(0);
}

