#ifndef _H264ENCODER_H
#define _H264ENCODER_H

#include <stdint.h>
#include <stdio.h>
#include <x264.h>

#ifdef __cplusplus
extern "C"
{
#endif
int x264_param_default_preset(x264_param_t*, char const*, char const*);
int x264_param_apply_profile(x264_param_t*, char const*);
int x264_picture_alloc(x264_picture_t*, int, int, int);
x264_t *x264_encoder_open( x264_param_t * );
void x264_picture_clean(x264_picture_t*);
int x264_encoder_encode(x264_t*, x264_nal_t**, int*, x264_picture_t*, x264_picture_t*);
void x264_encoder_close(x264_t*);
#ifdef __cplusplus
}
#endif

typedef unsigned char uint8_t;

typedef struct {
	x264_param_t *param;
	x264_t *handle;
	x264_picture_t *picture; //说明一个视频序列中每帧特点
	x264_nal_t *nal;
	int status; // 1 ok , 0 no init , -1 error
} Encoder;

//初始化编码器，并返回一个编码器对象
int compress_begin(Encoder *en, int width, int height);

//编码一帧
int compress_frame(Encoder *en, int type, char *in,int in_len, char *out);

//释放内存
void compress_end(Encoder *en);
int is_x264_coder_ok(Encoder *en);
#endif

