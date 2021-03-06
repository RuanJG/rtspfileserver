#include "Rtsp.h"
#include "Rtp.h"
#include "config.h"
#include "video_capture.h"

using namespace std;

//RTSP傳輸所需檔案
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
//RTSP的傳收BUFFER
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
	//存放OPTIONS DESCRIBE SETUP PLAY TEARDOWN等
	char *RequestType;
	char *RtspUrl;
	//TCP連線所需
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
			close(clientFD);//關閉Socket
			clientFD = 0;
			log_msg("Server is listening.....\n");
			clientFD = accept(serverFD,(struct sockaddr*)&addrClient,&addrClientLen);
			if(clientFD == -1){
				perror("Accept failed!!\n");
				close(serverFD);//關閉Socket
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
			close(clientFD);//關閉Socket
			clientFD = 0;
			log_msg("Server is listening.....\n");
			clientFD = accept(serverFD,(struct sockaddr*)&addrClient,&addrClientLen);
			if(clientFD == -1){
				perror("Accept failed!!\n");
				close(serverFD);//關閉Socket
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

	//建立Socket
	*serverFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	//Server Socket IP
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(RtspServerPort);

	//綁定Socket
	if(bind(*serverFD,(sockaddr*)&addrServer,sizeof(addrServer)) == -1){
		perror("Bind failed!\n");
		close(*serverFD);//關閉Socket
		*serverFD = 0;
		exit(0);
	}
	//建立監聽
	if(listen(*serverFD,10) == -1){
		perror("Listen failed!!\n");
		close(*serverFD);//關閉Socket
		*serverFD = 0;
		exit(0);
	}
	//接收客戶端請求
	log_msg("Server is listening %d .....",RtspServerPort);
	*clientFD = accept(*serverFD,(sockaddr*)addrClient,&addrClientLen);
	if(*clientFD == -1){
		perror("Accept failed!!\n");
		close(*serverFD);//關閉Socket
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
		res = pthread_create(&tid,NULL,Rtp_camera,(void*)&cam);
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
		close(clientFD);//關閉Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);

}
void DESCRIBE_Reply(int clientFD,char *RtspContentBase)
{
	char *RtspContentType = "Content-type: application/sdp\r\n";

#ifdef USE_X264_CODER
	char *SDPFile = "v=0\r\no=- 15409869162442327530 15409869162442327530 IN IP4 ESLab-PC\r\n"
					"s=Unnamed\r\ni=N/A\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.7\r\n"
					"a=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.2.1:8554/trackID=0\r\nm=video 20000 RTP/AVP 96\r\n"
					"b=RR:0\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=64001f;sprop-parameter-sets=Z2QAH6zZQFAFuwEQACi7EAmJaAjxgjlg,aOvjyyLA;\r\n"
					"a=control:rtsp://192.168.2.1:8554/trackID=1\r\n";
				    "Date: Wed, 15 May 2013 12:10:17 GMT\r\nContent-type: application/sdpContent-Base: rtsp://192.168.2.1:8554/\r\n"
					"Content-length: 362Cache-Control: no-cache\r\nCseq: 3\r\n\r\n"
					"v=0\r\no=- 15365712008849713956 15365712008849713956 IN IP4 User-PC\r\ns=Unnamed\r\ni=N/A\r\n"
					"c=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.3\r\na=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.2.1:8554/\r\na=framerate:25\r\nm=video 20000 RTP/AVP 96\r\nb=RR:0\r\n"
					"a=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1\r\na=control:rtsp://192.168.2.1:8554/trackID=1\r\n";
#else
	char *SDPFile = "v=0\r\no=- 15409869162442327530 15409869162442327530 IN IP4 ESLab-PC\r\n"
					"s=Unnamed\r\ni=N/A\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.7\r\n"
					"a=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.2.1:8554/trackID=0\r\nm=video 20000 RTP/AVP 26\r\n"
					"b=RR:0\r\na=rtpmap:26 JPEG/90000\r\na=fmtp:26 packetization-mode=1;profile-level-id=64001f;sprop-parameter-sets=Z2QAH6zZQFAFuwEQACi7EAmJaAjxgjlg,aOvjyyLA;\r\n"
					"a=control:rtsp://192.168.2.1:8554/trackID=1\r\n";
				    "Date: Wed, 15 May 2013 12:10:17 GMT\r\nContent-type: application/sdpContent-Base: rtsp://192.168.2.1:8554/\r\n"
					"Content-length: 362Cache-Control: no-cache\r\nCseq: 3\r\n\r\n"
					"v=0\r\no=- 15365712008849713956 15365712008849713956 IN IP4 User-PC\r\ns=Unnamed\r\ni=N/A\r\n"
					"c=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.3\r\na=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.2.1:8554/\r\na=framerate:25\r\nm=video 20000 RTP/AVP 26\r\nb=RR:0\r\n"
					"a=rtpmap:26 JPEG/90000\r\na=fmtp:26 packetization-mode=1\r\na=control:rtsp://192.168.2.1:8554/trackID=1\r\n";
#endif

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
		close(clientFD);//關閉Socket
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
		close(clientFD);//關閉Socket
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
	//RTP所需參數
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
		close(clientFD);//關閉Socket
		exit(0);
	}
	lock = 1;
	bzero(sendBuf,BUF_SIZE);
	//送完PLAY_Reply後開始進行RTP傳輸
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
		close(clientFD);//關閉Socket
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
		close(clientFD);//關閉Socket
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
