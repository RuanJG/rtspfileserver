#include "Rtsp.h"
#include "Rtp.h"






#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
#include <camkit.h>
#ifdef __cplusplus
}
#endif

#define MAX_RTP_SIZE 1420
FILE *outfd = NULL;
int quit = 0;
int debug = 0;

void quit_func(int sig)
{
	quit = 1;
}

int simple_demo(char * ip, int port);
int ck_demo(char * ip, int port);



int cktool_demo(char * ip, int port)
{
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
	int stage = 15;

	U32 vfmt = V4L2_PIX_FMT_YUYV;
	U32 ofmt = V4L2_PIX_FMT_YUV420;

	// set default values
	capp.dev_name = "/dev/video0";
	capp.width = 640;
	capp.height = 480;
	capp.pixfmt = vfmt;
	capp.rate = 15;

	cvtp.inwidth = 640;
	cvtp.inheight = 480;
	cvtp.inpixfmt = vfmt;
	cvtp.outwidth = 640;
	cvtp.outheight = 480;
	cvtp.outpixfmt = ofmt;

	encp.src_picwidth = 640;
	encp.src_picheight = 480;
	encp.enc_picwidth = 640;
	encp.enc_picheight = 480;
	encp.chroma_interleave = 0;
	encp.fps = 15;
	encp.gop = 12;
	encp.bitrate = 1000;

	pacp.max_pkt_len = 1400;
	pacp.ssrc = 1234;

	netp.serip = ip;
	netp.serport = port;
	netp.type = UDP;


	caphandle = capture_open(capp);
	if (!caphandle)
	{
		log_msg("--- Open capture failed\n");
		return -1;
	}

	//if ((stage & 0b00000001) != 0)
	{
		cvthandle = convert_open(cvtp);
		if (!cvthandle)
		{
			log_msg("--- Open convert failed\n");
			return -1;
		}
	}

	//if ((stage & 0b00000010) != 0)
	{
		enchandle = encode_open(encp);
		if (!enchandle)
		{
			log_msg("--- Open encode failed\n");
			return -1;
		}
	}

	//if ((stage & 0b00000100) != 0)
	{
		pachandle = pack_open(pacp);
		if (!pachandle)
		{
			log_msg("--- Open pack failed\n");
			return -1;
		}
	}

	//if ((stage & 0b00001000) != 0)
	{
		if (netp.serip == NULL || netp.serport == -1)
		{
			log_msg(
					"--- Server ip and port must be specified when using network\n");
			return -1;
		}

		nethandle = net_open(netp);
		if (!nethandle)
		{
			log_msg("--- Open network failed\n");
			return -1;
		}
	}

	// start capture encode loop
	int ret;
	void *cap_buf, *cvt_buf, *hd_buf, *enc_buf;
	char *pac_buf = (char *) malloc(MAX_RTP_SIZE);
	int cap_len, cvt_len, hd_len, enc_len, pac_len;
	enum pic_t ptype;
	struct timeval ctime, ltime;
	unsigned long fps_counter = 0;
	int sec, usec;
	double stat_time = 0;

	capture_start(caphandle);		// !!! need to start capture stream!
	quit = 0;
	gettimeofday(&ltime, NULL);
	while (!quit)
	{
		if (1)		// print fps
		{
			gettimeofday(&ctime, NULL);
			sec = ctime.tv_sec - ltime.tv_sec;
			usec = ctime.tv_usec - ltime.tv_usec;
			if (usec < 0)
			{
				sec--;
				usec = usec + 1000000;
			}
			stat_time = (sec * 1000000) + usec;		// diff in microsecond

			if (stat_time >= 1000000)    // >= 1s
			{
				log_msg("\n*** FPS: %ld\n", fps_counter);

				fps_counter = 0;
				ltime = ctime;
			}
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
				usleep(10000);
				continue;
			}
		}
		if (cap_len <= 0)
		{
			log_msg("!!! No capture data\n");
			continue;
		}


		// convert
		if (capp.pixfmt == V4L2_PIX_FMT_YUV420)    // no need to convert
		{
			cvt_buf = cap_buf;
			cvt_len = cap_len;
		}
		else	// do convert: YUYV => YUV420
		{
			ret = convert_do(cvthandle, cap_buf, cap_len, &cvt_buf, &cvt_len);
			if (ret < 0)
			{
				log_msg("--- convert_do failed\n");
				break;
			}
			if (cvt_len <= 0)
			{
				log_msg("!!! No convert data\n");
				continue;
			}
		}

		// encode
		// fetch h264 headers first!
		while ((ret = encode_get_headers(enchandle, &hd_buf, &hd_len, &ptype))
				!= 0)
		{
			if (debug)
				fputc('S', stdout);

			if ((stage & 0b00000100) == 0)		// no pack
			{
				if (outfd)
					fwrite(hd_buf, 1, hd_len, outfd);

				continue;
			}

			// pack headers
			pack_put(pachandle, hd_buf, hd_len);
			while (pack_get(pachandle, pac_buf, MAX_RTP_SIZE, &pac_len) == 1)
			{
				if (debug)
					fputc('#', stdout);

				if ((stage & 0b00001000) == 0)    // no network
				{
					if (outfd)
						fwrite(pac_buf, 1, pac_len, outfd);

					continue;
				}

				// network
				ret = net_send(nethandle, pac_buf, pac_len);
				if (ret != pac_len)
				{
					log_msg("send pack failed, size: %d, err: %s\n", pac_len,
							strerror(errno));
				}
				if (debug)
					fputc('>', stdout);
			}
		}

		ret = encode_do(enchandle, cvt_buf, cvt_len, &enc_buf, &enc_len,
				&ptype);
		if (ret < 0)
		{
			log_msg("--- encode_do failed\n");
			break;
		}
		if (enc_len <= 0)
		{
			log_msg("!!! No encode data\n");
			continue;
		}


		// pack
		pack_put(pachandle, enc_buf, enc_len);
		while (pack_get(pachandle, pac_buf, MAX_RTP_SIZE, &pac_len) == 1)
		{
			if (debug)
				fputc('#', stdout);

			if ((stage & 0b00001000) == 0)    // no network
			{
				if (outfd)
					fwrite(pac_buf, 1, pac_len, outfd);

				continue;
			}

			// network
			ret = net_send(nethandle, pac_buf, pac_len);
			if (ret != pac_len)
			{
				log_msg("send pack failed, size: %d, err: %s\n", pac_len,
						strerror(errno));
			}
			if (debug)
				fputc('>', stdout);
		}
		fps_counter++;
	}
	capture_stop(caphandle);

	free(pac_buf);
	if ((stage & 0b00001000) != 0)
		net_close(nethandle);
	if ((stage & 0b00000100) != 0)
		pack_close(pachandle);
	if ((stage & 0b00000010) != 0)
		encode_close(enchandle);
	if ((stage & 0b00000001) != 0)
		convert_close(cvthandle);
	capture_close(caphandle);

	//if (outfd)
	//	fclose(outfd);
	return 0;
}




int main(int args,char *argv[])
{
// rtspfile 640 480
// rtspfile test.264

	//signal(SIGINT, quit_func);
//	simple_demo(argv[1],2000);
	//ck_demo(argv[1],2000);
	Rtsp(args,argv);

	return 0;
}


