#include "Rtp.h"
#include "video_capture.h"

using namespace std;

char *FileTemp;
char *RtpHeader;
char *FUIndicator;
char *FUHeader;





void *Rtp(void *fileName)
{
	int sockFD;
	struct sockaddr_in addrClient;
	//RtpHeader相關參數
	int SequenceNumber = 6;
	unsigned int timestamp = 2785272748;
	unsigned int ssrc = 0xc630ebd7;
	//FileTemp檔相關參數
	char *FrameStartIndex;
	int FrameLength = 0;
	int FileSize;
	int ret;

	printf("I'm at Rtp()!\n");
	
	//建立RtpSocket
	createRtpSocket(&sockFD,&addrClient);
	//開啟H.264影像編碼檔並取得檔案大小
	FileSize = OpenVideoFile((char*)fileName);
	//創造與設定RTP標頭檔
	createRtpHeader();
	setRtpVersion(); 
	setRtpPayloadType();
	setSequenceNumber(SequenceNumber);
	setTimestamp(timestamp);
	setSSRC(ssrc);

	printf("Rtplock=%d\n",lock);
	for(int i=0;i<FileSize && lock;i++){
		//H.264 StartCode 為00 00 00 01或00 00 01
		if(*((int*)(FileTemp+i))==0x01000000){//轉型為4bytes
			//if((*((int*)(FileTemp+i))&0x00FFFFFF)==0x00010000) continue;
			if(FrameLength>0)
				RtpEncoder(sockFD,addrClient,FrameStartIndex,FrameLength,&SequenceNumber,&timestamp);
			FrameStartIndex = FileTemp + i;
			FrameLength = 0;
			i++;//StartCode(0x00010000)
			FrameLength++;
			//printf("FrameStartIndex=%X\n",*((int*)FrameStartIndex));
		}else if((*((int*)(FileTemp+i))&0x00FFFFFF)==0x00010000){
			if(FrameLength>0)
				RtpEncoder(sockFD,addrClient,FrameStartIndex,FrameLength,&SequenceNumber,&timestamp);
			FrameStartIndex = FileTemp + i;
			FrameLength = 0;
			//printf("FrameStartIndex!!!=%X\n",*((int*)FrameStartIndex));
		}
		FrameLength++;
	}
	printf("Rtplock=%d\n",lock);
	printf("End\n");
	close(sockFD);//關閉Socket
	free(FileTemp);
	free(RtpHeader);
	free(FUIndicator);
	free(FUHeader);
}

void *Rtp_camera(void *came)
{
	int sockFD;
	struct sockaddr_in addrClient;
	//RtpHeader相關參數
	int SequenceNumber = 26074;
	unsigned int timestamp = 2785272748;
	unsigned int ssrc = 0xc630ebd7;
	//FileTemp檔相關參數
	char *FrameStartIndex;
	int FrameLength = 0;
	int FileSize;

	struct camera *cam = (struct camera *)came;
	char *bigbuffer;
	int bigbuffer_szie;
	int pic_len;
	int ret;
	

	printf("I'm at Rtp()!\n");
	
	//建立RtpSocket
	createRtpSocket(&sockFD,&addrClient);
	//開啟H.264影像編碼檔並取得檔案大小
	//FileSize = OpenVideoFile((char*)fileName);
	//創造與設定RTP標頭檔
	createRtpHeader();
	setRtpVersion(); 
	setRtpPayloadType();
	setSequenceNumber(SequenceNumber);
	setTimestamp(timestamp);
	setSSRC(ssrc);


	//camera 
	v4l2_init(cam);

	bigbuffer_szie = cam->width*cam->height*MY_V4L2_BUFFER_COUNT*sizeof(char);
	bigbuffer = (char *)malloc(bigbuffer_szie);

	log_msg("the bigbuffer_szie=%d\n",bigbuffer_szie);
	printf("Rtplock=%d\n",lock);
	//for(int i=0;i<FileSize && lock;i++){
	while(lock){
		pic_len = 0;
#ifdef USE_X264_CODER
		pic_len = read_encode_frame(cam,bigbuffer,bigbuffer_szie);
#else
		pic_len = read_frame(cam,bigbuffer,bigbuffer_szie);
#endif

		ret = pic_len;
		if( ret == 0 ){
			log_msg("read no data frome camera\n");
			continue;
		}else if( ret < 0 ){
			log_msg("read error frome camera\n");
			break;
		}

		log_msg("read one data frome camera, size is %d, the bigbuffer_szie=%d\n",pic_len,bigbuffer_szie);
		if( pic_len >0){
			if( pic_len > bigbuffer_szie ){
				printf("RUAN: get a picture size is %d, bigger than %d  !!!!!!!!!!! \n",pic_len,bigbuffer_szie);
				pic_len = bigbuffer_szie; //should be error
			}
			printf("RUAN: get a pic_len = %d, Headeris %x\n",pic_len,*((int*)bigbuffer));
#ifdef USE_X264_CODER
			ret = RtpEncoder(sockFD,addrClient,bigbuffer,pic_len,&SequenceNumber,&timestamp);
#else
			ret = RtpJpegEncoder(sockFD ,addrClient,( unsigned char *)bigbuffer , pic_len , cam->width, cam->height);
#endif
			if( ret < 0 )
				break;
		}else{
			continue; // should not come here
		}

	}
	printf("Rtplock=%d\n",lock);
	printf("End\n");

	v4l2_exit(cam);

	close(sockFD);//關閉Socket
	free(bigbuffer);
	free(RtpHeader);
	free(FUIndicator);
	free(FUHeader);
}



void * rtp_worker(void *came)
{
	int sockFD;
	struct sockaddr_in addrClient;
	//RtpHeader相關參數
	int SequenceNumber = 26074;
	unsigned int timestamp = 2785272748;
	unsigned int ssrc = 0xc630ebd7;

	struct camera *cam = (struct camera *)came;
	cameraBuffer *camBuff = &cam->camBuff;
	camera_buffer_t * fram_buff;

	char *bigbuffer;
	int bigbuffer_szie;
	int pic_len;
	int ret;
	

	printf("I'm at Rtp()!\n");
	
	//建立RtpSocket
	createRtpSocket(&sockFD,&addrClient);
	//開啟H.264影像編碼檔並取得檔案大小
	//FileSize = OpenVideoFile((char*)fileName);
	//創造與設定RTP標頭檔
	createRtpHeader();
	setRtpVersion(); 
	setRtpPayloadType();
	setSequenceNumber(SequenceNumber);
	setTimestamp(timestamp);
	setSSRC(ssrc);

	log_msg("rtp_worker start \n");
	//for(int i=0;i<FileSize && lock;i++){
	while(lock){

		fram_buff = camBuff->get_value_buffer();
		if( fram_buff == NULL ){
			log_msg("rtp_work: can not get value_data\n");
			usleep(20000);
			continue;
		}
		pic_len = fram_buff->data_len;
		bigbuffer_szie = fram_buff->length;
		bigbuffer = fram_buff->start;


		log_msg("read one data frome camera, size is %d, the bigbuffer_szie=%d\n",pic_len,bigbuffer_szie);
		if( pic_len >0){
			printf("RUAN: get a pic_len = %d, Headeris %x\n",pic_len,*((int*)bigbuffer));
#ifdef USE_X264_CODER
			ret = RtpEncoder(sockFD,addrClient,bigbuffer,pic_len,&SequenceNumber,&timestamp);
#else
			ret = RtpJpegEncoder(sockFD ,addrClient,( unsigned char *)bigbuffer , pic_len , cam->width, cam->height);
#endif

			if( ret < 0 )
				break;
		}else{
			camBuff->return_buffer(fram_buff);
			continue; // should not come here
		}
		camBuff->return_buffer(fram_buff);

	}
	printf("Rtplock=%d\n",lock);
	printf("End\n");


	close(sockFD);//關閉Socket
	free(RtpHeader);
	free(FUIndicator);
	free(FUHeader);
}

void *camera_worker(void *came)
{
	struct camera *cam = (struct camera *)came;
	char *bigbuffer;
	int bigbuffer_szie;
	int pic_len;
	int ret;
	cameraBuffer *camBuff = &cam->camBuff;
	camera_buffer_t * fram_buff=NULL;

	log_msg("camera_worker start \n");
	v4l2_init(cam);
	while( lock){
		if( fram_buff == NULL )
			fram_buff = camBuff->get_empty_buffer();

		if( fram_buff == NULL ){
			log_msg("camera work: can not get the empty buffer\n");
			usleep(10000);
			continue;
		}

		pic_len = 0; //fram_buff->data_len;
		bigbuffer_szie = fram_buff->length;
		bigbuffer = fram_buff->start;

#ifdef USE_X264_CODER
		pic_len = read_encode_frame(cam,bigbuffer,bigbuffer_szie);
#else
		pic_len = read_frame(cam,bigbuffer,bigbuffer_szie);
#endif

		ret = pic_len;

		if( ret == 0 ){
			log_msg("read no data frome camera\n");
			continue;
		}else if( ret < 0 ){
			log_msg("read error frome camera\n");
			break;
		}
		
		fram_buff->data_len = pic_len;
		camBuff->set_valid_buffer(fram_buff);
		fram_buff = NULL;
	}

	v4l2_exit(cam);
}


void createRtpSocket(int *sockFD,struct sockaddr_in *addrClient)
{
	struct sockaddr_in addrServer;
	int addrClientLen = sizeof(addrClient);

	//建立Socket
	*sockFD = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(*sockFD == -1){
		perror("Socket failed!!\n");
		exit(0);
	} 
	//Server Socket 位址
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(RtpParameter.rtpServerPort);
	//Client Socket 位址
	addrClient->sin_family = AF_INET;
	addrClient->sin_addr.s_addr = RtpParameter.addrClient.sin_addr.s_addr;
	addrClient->sin_port = htons(RtpParameter.rtpClientPort);

	printf("PORT : rtp send frome S:%d to C:%d\n",RtpParameter.rtpServerPort,RtpParameter.rtpClientPort);

	//printf("IP=%s\n",inet_ntoa(addrClient->sin_addr));
	//printf("Port=%d\n",ntohs(addrClient->sin_port));
	if(bind(*sockFD,(sockaddr*)&addrServer,sizeof(addrServer)) == -1){
		perror("Bind failed!\n");
		close(*sockFD);//關閉Socket
		exit(0);
	}
}
int OpenVideoFile(char *fileName)
{
	FILE *fPtr;
	int FileSize;

	fPtr = fopen(fileName,"rb");
	if(!fPtr){
		perror("Open failed!");
		exit(0);
	}
	//find the fileSize
	fseek(fPtr,0,SEEK_END);
	FileSize = ftell(fPtr);
	fseek(fPtr,0,SEEK_SET);
	//printf("fileSize=%d bytes\n",FileSize);
	
	FileTemp = (char*)malloc(FileSize*sizeof(char));
	//複製檔案內容
	if(fPtr)
		fread(FileTemp,1,FileSize,fPtr);
	else 
		perror("File Open Error!"),exit(0);

	fclose(fPtr);
	return FileSize;
}
//與RtpHeader有關的參數與設定
void createRtpHeader()
{
	int RtpHeaderSize = 12;
	RtpHeader = (char*)malloc(RtpHeaderSize*sizeof(char));
	bzero(RtpHeader,RtpHeaderSize);
}
void setRtpVersion()
{
	RtpHeader[0] |= 0x80;
}
void setRtpPayloadType()
{
	RtpHeader[1] |= PayloadType;
}
void setSequenceNumber(int SequenceNumber)
{
	RtpHeader[2] &=0;
	RtpHeader[3] &=0;
	//取得上半部8bits的位元並將int轉型為char存進RtpHeader裡
	RtpHeader[2] |= (char)((SequenceNumber & 0x0000FF00)>>8);
	//取得下半部8bits的位元並將int轉型為char存進RtpHeader裡
	RtpHeader[3] |= (char)(SequenceNumber & 0x000000FF);
	//printf("%X\n%X\n", RtpHeader[2],RtpHeader[3]);
}
void setTimestamp(unsigned int timestamp)
{
	RtpHeader[4] &= 0;
	RtpHeader[5] &= 0;
	RtpHeader[6] &= 0;
	RtpHeader[7] &= 0;
	RtpHeader[4] |= (char)((timestamp & 0xff000000) >> 24);
	RtpHeader[5] |= (char)((timestamp & 0x00ff0000) >> 16);
	RtpHeader[6] |= (char)((timestamp & 0x0000ff00) >> 8);
	RtpHeader[7] |= (char)(timestamp & 0x000000ff);
	/*printf("setTimestamp=%X\n", RtpHeader[4] );
	printf("%X\n", RtpHeader[5] );
	printf("%X\n", RtpHeader[6] );
	printf("%X\n", RtpHeader[7] );*/
}
void setSSRC(unsigned int ssrc)
{
	RtpHeader[8]  &= 0;
	RtpHeader[9]  &= 0;
	RtpHeader[10] &= 0;
	RtpHeader[11] &= 0;
	RtpHeader[8]  |= (char) ((ssrc & 0xff000000) >> 24);
	RtpHeader[9]  |= (char) ((ssrc & 0x00ff0000) >> 16);
	RtpHeader[10] |= (char) ((ssrc & 0x0000ff00) >> 8);
	RtpHeader[11] |= (char) (ssrc & 0x000000ff);
	/*printf("%X\n", RtpHeader[8] );
	printf("%X\n", RtpHeader[9] );
	printf("%X\n", RtpHeader[10] );
	printf("%X\n", RtpHeader[11] );*/
}
void setMarker(int marker)
{
	if(marker == 0){
		RtpHeader[1] &= 0x7f;
	}else{
		RtpHeader[1] |= 0x80;
	}
}

//生成傳輸Rtp封包所需格式
int RtpEncoder(int sockFD,struct sockaddr_in addrClient,char *frame_head,int len,int *SequenceNumber,unsigned int *timestamp)
{
	//先將長度扣除StartCode部分
	int ret;
	int FrameLength = len;
	char *FrameStartIndex = frame_head;

	if(*((int*)FrameStartIndex) == 0x01000000) FrameLength -= 4;
	else  FrameLength -= 3;

	
	//原始封包規格[Start code][NALU][Raw Data]
	if(FrameLength < 1400){ 
		log_msg("buff is little than 1400\n");
		//封包長度小於MTU(減去其他層Header)
		char *sendBuf = (char*)malloc((FrameLength+12)*sizeof(char));
		
		//[RTP Header][NALU][Raw Date]
		if(*((int*)FrameStartIndex) == 0x01000000) 
			memcpy(sendBuf+12,FrameStartIndex+4,FrameLength);
		else
			memcpy(sendBuf+12,FrameStartIndex+3,FrameLength);

		//WiredShark顯示以67 68開頭的資料Mark等於0
		if(sendBuf[12] == 0x67 || sendBuf[12] == 0x68){
			setMarker(0);
			memcpy(sendBuf,RtpHeader,12);
		}else{
			setMarker(1);
			memcpy(sendBuf,RtpHeader,12);
			//設定timestamp以(90000/fps)遞增
			(*timestamp) += (90000/fps);
			//(*timestamp) += convertToRTPTimestamp();
			setTimestamp(*timestamp);
		}
		
		if(sendto(sockFD,sendBuf,FrameLength+12,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
			printf("Sent failed!!\n");
			close(sockFD);//關閉Socket
			free(sendBuf);
			return -1;
		}
		usleep(sleepTime);
		//封包傳輸序列遞增
		(*SequenceNumber)++;
		setSequenceNumber(*SequenceNumber);
		free(sendBuf);
		//printf("FrameLength=%d\n",FrameLength);
		//printf("FrameStartIndex1=%X\n",*((int*)FrameStartIndex));
	}else{
		log_msg("buff is bigger than 1400\n");
		//[RTP Header][FU indicator][FU header][Raw Data]
		FUIndicator = (char*)malloc(sizeof(char));
		FUHeader = (char*)malloc(sizeof(char));
		if(FrameLength >= 1400){
			setMarker(0);
			setFUIndicator(FrameStartIndex);
			setFUHeader(FrameStartIndex,1,0);//第一個分段包
			char *sendBuf = (char *)malloc(1500*sizeof(char));
			memcpy(sendBuf,RtpHeader,12);//[RTP Header]
			memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
			memcpy(sendBuf+13,FUHeader,1);//[FU header]

			if(FrameStartIndex[3] == 0x01){//00 00 00 01
				memcpy(sendBuf+14,FrameStartIndex+5,1386);//[raw data]
				FrameLength -= 1387;//包含NALU(1 Byte)
				FrameStartIndex += 1391;//包含startCode(4 Bytes)和NALU(1 Byte)
				//printf("FrameStartIndex2=%X\n",*((int*)FrameStartIndex));
			}else if(FrameStartIndex[2] == 0x01){//00 00 01
				memcpy(sendBuf+14,FrameStartIndex+4,1386);//[raw data]
				FrameLength -= 1387;
				FrameStartIndex += 1390;//包含startCode(3 Bytes)和NALU(1 Byte)
				//printf("FrameStartIndex3=%X\n",*((int*)FrameStartIndex));
			}
			if(sendto(sockFD,sendBuf,1400,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
				printf("Sent failed!!\n");
				close(sockFD);//關閉Socket
				free(sendBuf);
			}
			usleep(sleepTime);
			//封包傳輸序列遞增
			(*SequenceNumber)++;
			setSequenceNumber(*SequenceNumber);
			
			while(FrameLength >= 1400){
				setFUIndicator(FrameStartIndex);
				setFUHeader(FrameStartIndex,0,0);
				memcpy(sendBuf,RtpHeader,12);//[RTP Header]
				memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
				memcpy(sendBuf+13,FUHeader,1);//[FU header]
				memcpy(sendBuf+14,FrameStartIndex,1386);
				FrameLength -= 1386;
				FrameStartIndex +=1386;
				//printf("FrameStartIndex4=%X\n",*((int*)FrameStartIndex));
				log_msg("send one package to client, FrameLength=%d\n",FrameLength);
				if(sendto(sockFD,sendBuf,1400,0,(sockaddr *)&addrClient,sizeof(addrClient)) <= 0){
					printf("Sent failed!!\n");
					close(sockFD);//關閉Socket
					free(sendBuf);
					FrameLength = 0;
					return -1;
				}
				usleep(sleepTime);
				//封包傳輸序列遞增
				(*SequenceNumber)++;
				setSequenceNumber(*SequenceNumber);
			}
			if(FrameLength > 0){ //資料分段的最後一個封包
				setFUIndicator(FrameStartIndex);
				setFUHeader(FrameStartIndex,0,1);
				setMarker(1);
				memcpy(sendBuf,RtpHeader,12);//[RTP Header]
				memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
				memcpy(sendBuf+13,FUHeader,1);//[FU header]
				memcpy(sendBuf+14,FrameStartIndex,FrameLength);
				log_msg("send last one package to client\n");
				if(sendto(sockFD,sendBuf,FrameLength+14,0,(sockaddr *)&addrClient,sizeof(addrClient)) <=0){
					printf("Sent failed!!\n");
					close(sockFD);//關閉Socket
					free(sendBuf);
					return -1;
				}
				usleep(sleepTime);
				//printf("FrameStartIndex5=%X\n",*((int*)FrameStartIndex));
				//設定timestamp以(90000/fps)遞增
				(*timestamp) += (90000/fps);
				//(*timestamp) += convertToRTPTimestamp();
				setTimestamp(*timestamp);
				//封包傳輸序列遞增
				(*SequenceNumber)++;
				setSequenceNumber(*SequenceNumber);
			}
			free(sendBuf);
		}
	}	
	return 0;
}

int start_seq = 0;
unsigned short seq_num = 0;
unsigned int ts_current = 0;
unsigned char extractQTable1[64];
unsigned char extractQTable2[64];
#define PACKET_SIZE 1400
#define RTP_JPEG_RESTART 0x40
#define RTP_HDR_SZ       12
#define RTP_PT_JPEG      26
typedef struct {
        unsigned char cc:4;
        unsigned char x:1;
        unsigned char p:1;
        unsigned char version:2;
        unsigned char pt:7;
        unsigned char m:1;
        unsigned short seq;
        unsigned int ts;
        unsigned int ssrc;
}rtp_hdr_t;

struct jpeghdr {
    unsigned int tspec:8;   /* type-specific field */
    unsigned int off:24;    /* fragment byte offset */
    unsigned char type;            /* id of jpeg decoder params */
    unsigned char q; /* quantization factor (or table id) */
    unsigned char width;           /* frame width in 8 pixel blocks */
    unsigned char height;          /* frame height in 8 pixel blocks */

};
struct jpeghdr_rst{
    unsigned short dri;
    unsigned int f:1;
    unsigned int l:1;
    unsigned int count:14;
 };
struct jpeghdr_qtable {
    unsigned char  mbz;
    unsigned char  precision;
    unsigned short length;

};

static unsigned int convertToRTPTimestamp(/*struct timeval tv*/)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned int  timestampIncrement = (90000*tv.tv_sec);
    timestampIncrement += (unsigned int )((2.0*90000*tv.tv_usec + 1000000.0)/2000000);   
    unsigned int const rtpTimestamp = timestampIncrement;  
    return rtpTimestamp;
}
unsigned short SendFrame(unsigned short start_seq, 
                         unsigned int  ts,
                         unsigned int ssrc,
                         unsigned char *jpeg_data, 
                         int len, 
                         unsigned short type,
                         unsigned short typespec, 
                         int width, 
                         int height, 
                         unsigned short dri,
                         unsigned short q,
			 int sockfd,
                         struct sockaddr_in addrClient)
{
	rtp_hdr_t rtphdr;
	struct jpeghdr jpghdr;
	struct jpeghdr_rst rsthdr;
	struct jpeghdr_qtable qtblhdr;
	unsigned char packet_buf[PACKET_SIZE];
	unsigned char *ptr = NULL;
    unsigned int off = 0;
	int bytes_left = len;
	int seq = start_seq;
	int pkt_len, data_len;
	/* Initialize RTP header
	 */
	rtphdr.version = 2;
	rtphdr.p = 0;
	rtphdr.x = 0;
	rtphdr.cc = 0;
	rtphdr.m = 0;
	rtphdr.pt = RTP_PT_JPEG;
    rtphdr.seq = htons(seq_num);
	rtphdr.ts = htonl(convertToRTPTimestamp());
	rtphdr.ssrc = htonl(ssrc);

	/* Initialize JPEG header
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | Type-specific |              Fragment Offset                  |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |      Type     |       Q       |     Width     |     Height    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	jpghdr.tspec = typespec;
	jpghdr.off = off;
	jpghdr.type = type | ((dri != 0) ? RTP_JPEG_RESTART : 0);
    
	jpghdr.q = q;
	jpghdr.width = width/8;
	jpghdr.height = height/8;
	/* Initialize DRI header
    //     0                   1                   2                   3
    //0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //|       Restart Interval        |F|L|       Restart Count       |
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	 */
	if (dri != 0) {
		rsthdr.dri = dri;
		rsthdr.f = 1;        /* This code does not align RIs */
		rsthdr.l = 1;
		rsthdr.count = 0x3fff;
	}

	/* Initialize quantization table header
	 */
	if (q >= 80) {
		qtblhdr.mbz = 0;
		qtblhdr.precision = 0; /* This code uses 8 bit tables only */
		qtblhdr.length = 128;  /* 2 64-byte tables */
	}


	while (bytes_left > 0) {
		ptr = packet_buf + RTP_HDR_SZ;
		
        memcpy(ptr, &jpghdr, sizeof(jpghdr));
      	ptr += sizeof(jpghdr);
        rtphdr.m = 0;
        
        if (dri != 0) {
            memcpy(ptr, &rsthdr, sizeof(rsthdr));
            ptr += sizeof(rsthdr);
        }
        if (q >= 80 && jpghdr.off == 0) {
            memcpy(ptr, &qtblhdr, sizeof(qtblhdr));
            ptr += sizeof(qtblhdr);
            memcpy(ptr, extractQTable1, 64);
            ptr += 64;
            memcpy(ptr, extractQTable2, 64);
            ptr += 64;
        }

        data_len = PACKET_SIZE - (ptr - packet_buf);
        if (data_len >= bytes_left) {
            data_len = bytes_left;
            rtphdr.m = 1;
        }
        memcpy(packet_buf, &rtphdr, RTP_HDR_SZ);
        memcpy(ptr, jpeg_data + off, data_len);

        //send(sockfd,packet_buf, (ptr - packet_buf) + data_len,0);
        //send_sock(packet_buf,(ptr-packet_buf)+data_len);
	if(sendto(sockfd,packet_buf,(ptr-packet_buf)+data_len,0,(sockaddr *)&addrClient,sizeof(addrClient)) <=0){
		printf("Sent failed!!\n");
		close(sockfd);
		return -1;
	}
        off+=data_len;
        jpghdr.off = htonl(off);
        //jpghdr.off = convert24bit(off);
        //jpghdr.off = off;
        bytes_left -= data_len;
        rtphdr.seq = htons(++seq_num);
        
    }
    return rtphdr.seq;
}

static int RtpJpegEncoder(int sockfd, struct sockaddr_in addrClient,unsigned char *in,int outsize,int width,int height)
{
    
    unsigned char typemjpeg = 0;
    unsigned char typespecmjpeg = 1;
    unsigned char drimjpeg = 0;
    unsigned char qmjpeg = 70;
 //   extractQTable(in,outsize);
    start_seq = SendFrame(start_seq,ts_current,10, in,outsize,typemjpeg,typespecmjpeg,width,height,drimjpeg,qmjpeg, sockfd , addrClient);
    if( start_seq == -1 )
	    return -1;

	return 0;
}

void setFUIndicator(char *FrameStartIndex)
{
	int FUIndicatorType = 28;
	
	if(FrameStartIndex[2] == 0x01)
		FUIndicator[0] = (FrameStartIndex[3] & 0x60) | FUIndicatorType;
	else if(FrameStartIndex[3] == 0x01)
		FUIndicator[0] = (FrameStartIndex[4] & 0x60) | FUIndicatorType;
}
void setFUHeader(char *FrameStartIndex,bool start,bool end)
{
	if(FrameStartIndex[3] == 0x01)
		FUHeader[0] = FrameStartIndex[4] & 0x1f;
	else if(FrameStartIndex[2] == 0x01)
		FUHeader[0] = FrameStartIndex[3] & 0x1f;
	//設定FU-A Header的封包是第一個分段包或中間包或最後一包
	/*  
	   |0|1|2|3 4 5 6 7|
	   |S|E|R|  TYPE   | 
	*/
	if(start) FUHeader[0] |= 0x80;
	else FUHeader[0] &= 0x7f;
	if(end) FUHeader[0] |= 0x40;
	else FUHeader[0] &= 0xBF;
}
