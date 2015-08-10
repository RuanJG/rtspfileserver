/*************************************************************************
	> File Name: video_capture.h
	> Author: 
	> Mail: 
	> Created Time: 2015年01月23日 星期五 14时49分02秒
 ************************************************************************/

#ifndef _VIDEO_CAPTURE_H
#define _VIDEO_CAPTURE_H

#include <linux/videodev2.h>
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>


typedef struct __cam_buffer{
	char *start;
  	int length;
	int data_len;
	char is_empty;
	char id;
}camera_buffer_t;

class cameraBuffer{

#define BUFF_COUNT 3
#define UNVALID_INDEX -1
private:
	camera_buffer_t buffer[BUFF_COUNT];

	int valid_index_count;
	int valid_index_list[BUFF_COUNT];

	int status ;
	pthread_mutex_t mutex;

	void free_buff(){
		int i ;
		pthread_mutex_lock(&mutex);
		for ( i=0 ; i< BUFF_COUNT; i++ ){
			if( buffer[i].start != NULL ) free(buffer[i].start);
			buffer[i].is_empty = 0;
			buffer[i].start = NULL;
		}
		valid_index_count = 0;
		status = -1;
		pthread_mutex_unlock(&mutex);
	}

	int get_empty_index()
	{
		int i;
		for( i = 0; i< BUFF_COUNT; i ++ ){
			if( buffer[i].is_empty ){
				return i;
			}
		}
		return UNVALID_INDEX;
	}

	int get_value_index()
	{
		int  index;
		if( valid_index_count <= 0 ) return UNVALID_INDEX;	
		index = valid_index_list[0];

		return index;
	}

	int is_status_ok()
	{
		return status == 0 ? 1:0;
	}


public:
	// camera: get_empty_buffer -> set_valid_buffer
	// rtp: get_value_buffer -> return_buffer
	int set_valid_buffer(camera_buffer_t * buff)
	{
		int i,ret;
		pthread_mutex_lock(&mutex);
		if( !is_status_ok() ){
			pthread_mutex_unlock(&mutex);
			return -1;
		}

		if( valid_index_count > BUFF_COUNT )
			ret = -1;
		else{
			valid_index_count++;
			valid_index_list[valid_index_count-1] = buff->id;
			ret = 0;
		}

		pthread_mutex_unlock(&mutex);
		log_msg("add valid Buffer : I have %d valid buffer\n",valid_index_count);
		return ret;
	}
	camera_buffer_t *get_value_buffer()
	{
		int index, ret,i;
		camera_buffer_t *buff= NULL;

		pthread_mutex_lock(&mutex);
		if( !is_status_ok() ){
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		index = get_value_index();
		if(index == UNVALID_INDEX ){
			log_msg("no valid buffer !!!!!\n");
		}else{
			buff = & buffer[index];
			valid_index_count--;
			for( i = 0; i<  valid_index_count ; i++)
			{
				valid_index_list[i] = valid_index_list[i+1];
			}
		}
		log_msg("give valid Buffer : I still have %d valid buffer\n",valid_index_count);
		pthread_mutex_unlock(&mutex);
		return buff;
	}
	void return_buffer(camera_buffer_t * buff)
	{
		int i ;

		pthread_mutex_lock(&mutex);
		if( !is_status_ok() ){
			pthread_mutex_unlock(&mutex);
			return ;
		}
		buff->is_empty = 1;
		pthread_mutex_unlock(&mutex);
	}
	camera_buffer_t * get_empty_buffer()
	{
		int index,ret;
		camera_buffer_t * buff=NULL;

		pthread_mutex_lock(&mutex);

		if( !is_status_ok() ){
			pthread_mutex_unlock(&mutex);
			return NULL;
		}

		index = get_empty_index();
		if( index == UNVALID_INDEX ){
			log_msg("no empty buffer !!!!!\n");
		}else{
			buff = &buffer[index];
			buff->is_empty = 0;
		}

		pthread_mutex_unlock(&mutex);
		return buff;
	}

	int init(int buff_size)
	{
		int i,ret=0;

		valid_index_count = 0;
		pthread_mutex_init(&mutex,NULL);

		for( i = 0; i< BUFF_COUNT ; i++ ){
			valid_index_list[i] = UNVALID_INDEX;
			buffer[i].start = NULL;
			buffer[i].length = 0;
			buffer[i].data_len = 0;
			buffer[i].is_empty = 1;
			buffer[i].id = i;
		}
			
		for( i = 0; i< BUFF_COUNT ; i++ ){
			buffer[i].start = (char *)malloc(buff_size);
			if( buffer[i].start == NULL ){
				log_msg("the %d buff is malloc null\n",i);
				ret = -1;
				break;
			}
			buffer[i].length = buff_size;
		}
		if( ret == -1 )
			free_buff();
		status = ret;
		return ret;
	}
	
	void deinit()
	{
		if( !is_status_ok() ){
			return ;
		}
		free_buff();
	}



};


struct buffer{
    void *start;
    int length;
};
struct camera{
    char *device_name;
    int fd;
    int width;
    int height;
    int display_depth;
    int image_size;
    int frame_number;
    int support_fmt;
    unsigned int n_buffers;
    struct v4l2_capability v4l2_cap;
    struct v4l2_cropcap v4l2_cropcap;
    struct v4l2_format v4l2_fmt;
    struct v4l2_crop v4l2_crop;
    struct v4l2_fmtdesc v4l2_fmtdesc;
    struct v4l2_streamparm v4l2_setfps;
    struct buffer *buffers;

    cameraBuffer camBuff;

    int status; // if status == -1 , return
};
/*上面参数fmt_select选择*/
#define FMT_JPEG    0x101
#define FMT_YUYV422 0x102
#define FMT_YUYV420 0x104
/*辅助函数*/
//void errno_exit(const char *s);
//int xioctl(int fd,int request,void *arg);
/*对摄像头操作函数*/
//void open_camera(struct camera *cam);
//void close_camera(struct camera *cam);

//void init_camera(struct camera *cam);
//void exit_camera(struct camera *cam);

//void start_capturing(struct camera *cam);
//void stop_capturing(struct camera *cam);

/*视频数据读取处理函数
 * 此函数用于读取yuyv 或者yuv422数据 不能读取JPEG数据
 * buffer必须是已经分配好的内存 内存大小为cam->width*cam->height*3
 * 成功返回 0 继续读取返回1 错误返回-1
 *
 * */
int read_encode_frame(struct camera *cam,char *buffer,int len);
int read_frame(struct camera *cam,char *buffer,int max_len);
/*初始化退出总函数*/
void v4l2_init(struct camera *cam);
void v4l2_exit(struct camera *cam);

#endif
