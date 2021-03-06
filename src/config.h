/*************************************************************************
	> File Name: config.h
	> Author: 
	> Mail: 
	> Created Time: 2015年01月23日 星期五 15时19分47秒
 ************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H
/******************调试信息*****************************/
#define OUTPUT_CAMINFO/*输出摄像头的信息*/
//#define SAVEFIRSTJPEG/*输出第一副图片用于测试*/


/******************摄像头信息***************************/
#define CAM_WIDGH 1280 //640
#define CAM_HEIGHT  720 //480
// height widgh
#define CAM_DEVICE "/dev/video0"

/**********************dbug*********************************/

#if 0
    #define dbug(x) printf("%s %s %d %s\n",__FILE__,__func__,__LINE__,x)
#else
    #define dbug(x) 
#endif

#if 0
#define TIME_CHECK_FUNC
#define log_msg(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#else
#define log_msg(format, ...)
#endif

#define MY_V4L2_BUFFER_COUNT 4
//#define USE_X264_CODER
//#define USE_CAMERA_THREAD
#define FPS 30


#endif
