#ifndef _H264ENCODER_H
#define _H264ENCODER_H

/*
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
*/

#ifdef __cplusplus
extern "C"
{
#endif
//初始化编码器，并返回一个编码器对象
int h264_compress_begin(int width, int height);

//编码一帧
int h264_compress_frame(int type, char *in,int in_len, char *out);

//释放内存
void h264_compress_end();
int is_h264_coder_ok();
#ifdef __cplusplus
}
#endif



#endif

