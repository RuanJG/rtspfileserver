#include "Rtsp.h"
#include "Rtp.h"

int main(int args,char *argv[])
{
	printf("InputFileNumber:%d\n",args-1);
	printf("InputFileName:%s\n",argv[args-1]);
	Rtsp(args,argv);

	return 0;
}


