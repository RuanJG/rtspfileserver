/*************************************************************************
	> File Name: video_capture.c
	> Author: 
	> Mail: 
	> Created Time: 2015年01月23日 星期五 14时58分33秒
 ************************************************************************/
#include <asm/types.h>          /* for videodev2.h */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <linux/videodev2.h>
#include <dirent.h>
#include "video_capture.h"
#include "config.h"
#include "h264encoder.h"

#define CLEAR(x) memset (&(x), 0, sizeof (x))


/*辅助函数*/
static  void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg) {
    int r = 0;
    
    do{
        r = ioctl(fd, request, arg);
    }while (-1 == r && EINTR == errno);
    
    return r;
}
/*摄像头操作函数*/
static void open_camera(struct camera *cam)
{
    struct stat st;

    if (-1 == stat(cam->device_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", cam->device_name,errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", cam->device_name);
        exit(EXIT_FAILURE);
    }

    cam->fd = open(cam->device_name, O_RDWR, 0); //  | O_NONBLOCK

    if (-1 == cam->fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", cam->device_name, errno,strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void close_camera(struct camera *cam) 
{
    if (-1 == close(cam->fd))
        errno_exit("close");
    cam->fd = -1;
}
/*success pos,fail -1*/
static int find_string(const char *pSrc, const char *pDst)  
{  
    int i, j;  
    for (i=0; pSrc[i]!='\0'; i++){  
        if(pSrc[i]!=pDst[0])  
            continue;         
            j = 0;  
            while(pDst[j]!='\0' && pSrc[i+j]!='\0'){  
                j++;
                if(pDst[j]!=pSrc[i+j])
                    break;  
            }
            if(pDst[j]=='\0')
                return i;
    }  
    return -1;  
} 

static void init_camera(struct camera *cam)
{
    struct v4l2_capability  *cap        = &(cam->v4l2_cap);
    struct v4l2_cropcap     *cropcap    = &(cam->v4l2_cropcap);
    struct v4l2_crop        *crop       = &(cam->v4l2_crop);
    struct v4l2_format      *fmt        = &(cam->v4l2_fmt);
    struct v4l2_fmtdesc     *fmtdesc    = &(cam->v4l2_fmtdesc);
    struct v4l2_streamparm  *streamparm = &(cam->v4l2_setfps);
   
    struct v4l2_requestbuffers req;
    unsigned int min;
    int pos = -1;

    if (-1 == xioctl(cam->fd, VIDIOC_QUERYCAP, cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", cam->device_name);
            exit(EXIT_FAILURE);
        }else
            errno_exit("VIDIOC_QUERYCAP");
    }

    if (!(cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", cam->device_name);
        exit(EXIT_FAILURE);
    }

    if (!(cap->capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n",cam->device_name);
        exit(EXIT_FAILURE);
    }
#ifdef OUTPUT_CAMINFO
    printf("VIDOOC_QUERYCAP\n");
    printf("driver:\t\t%s\n",cap->driver);
    printf("card:\t\t%s\n",cap->card);
    printf("bus_info:\t%s\n",cap->bus_info);
    printf("version:\t%d\n",cap->version);
    printf("capabilities:\t%x\n",cap->capabilities);
#endif
    /*打印摄像头支持的格式*/
    fmtdesc->index = 0;
    fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#ifdef OUTPUT_CAMINFO
    printf("support format:\n");
#endif
    while(ioctl(cam->fd,VIDIOC_ENUM_FMT,fmtdesc) != -1){
#ifdef OUTPUT_CAMINFO
        printf("\t%d,%s\n",fmtdesc->index+1,fmtdesc->description);
#endif
        pos = find_string((char *)fmtdesc->description,"JPEG");
        if(pos != -1){
            cam->support_fmt |= FMT_JPEG;
            goto CON;
        }    
        pos = find_string((char *)fmtdesc->description,"4:2:2");
        if(pos!=-1){
            cam->support_fmt |= FMT_YUYV422;
            goto CON;
        }    
        pos = find_string((char *)fmtdesc->description,"4:2:0");
        if(pos!=-1){
            cam->support_fmt |= FMT_YUYV420;
            goto CON;
        }    
CON:
        fmtdesc->index++;
    }
    /*设置输出格式*/
    CLEAR(*cropcap);

    cropcap->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    crop->c.width = cam->width;
    crop->c.height = cam->height;
    crop->c.left = 0;
    crop->c.top = 0;
    crop->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   
    CLEAR(*fmt);
    fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt->fmt.pix.width = cam->width;
    fmt->fmt.pix.height = cam->height;

#ifdef USE_X264_CODER
    if((cam->support_fmt & FMT_YUYV422) == FMT_YUYV422){
        fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        cam->support_fmt = FMT_YUYV422;
	log_msg("camera use yuyu422 format\n");
    }
    else if( (cam->support_fmt & FMT_YUYV420 ) == FMT_YUYV420 ){
	log_msg("camera use yuv420 format\n");
        fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        cam->support_fmt = FMT_YUYV420;
    }else if( (cam->support_fmt & FMT_JPEG) == FMT_JPEG){
	log_msg("camera use jpeg format\n");
        fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        cam->support_fmt = FMT_JPEG;
    }
    else
        errno_exit("no support fmt");
#else
    if( (cam->support_fmt & FMT_JPEG) == FMT_JPEG){
	log_msg("camera use jpeg format\n");
        fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        cam->support_fmt = FMT_JPEG;
    }
    else
        errno_exit("no support fmt");
#endif
	

    fmt->fmt.pix.field = V4L2_FIELD_INTERLACED;
    
    if (-1 == xioctl(cam->fd,VIDIOC_S_FMT,fmt)){
        log_msg("!!!!!!!!!!!! VIDIOC_S_FMT fail");
    }
        
#if 1
    CLEAR(*streamparm);
    streamparm->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamparm->parm.capture.timeperframe.numerator   = 1;
    streamparm->parm.capture.timeperframe.denominator = 25;
    if(-1==xioctl(cam->fd,VIDIOC_S_PARM,streamparm))
        errno_exit("VIDIOC_S_PARM");
#endif
    min = fmt->fmt.pix.width * 2;/*YUYV 每个像素占用2个字节*/
    if (fmt->fmt.pix.bytesperline < min)
        fmt->fmt.pix.bytesperline = min;
    
    min = fmt->fmt.pix.bytesperline * fmt->fmt.pix.height;
    if (fmt->fmt.pix.sizeimage < min)
        fmt->fmt.pix.sizeimage = min;
    
    
    /*分配内存，映射内存*/
    CLEAR(req);
    req.count = MY_V4L2_BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    
    if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                    "memory mapping\n", cam->device_name);
            exit(EXIT_FAILURE);
        } else
            errno_exit("VIDIOC_REQBUFS");
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", cam->device_name);
        exit(EXIT_FAILURE);
    }

    cam->buffers = (struct buffer *)calloc(req.count, sizeof(*(cam->buffers)));

    if (!cam->buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
    for(cam->n_buffers = 0;cam->n_buffers < req.count;cam->n_buffers++){
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cam->n_buffers;

        if (-1 == xioctl(cam->fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");
        
        cam->buffers[cam->n_buffers].length = buf.length;
        cam->buffers[cam->n_buffers].start = mmap(NULL /* start anywhere */,buf.length, PROT_READ | PROT_WRITE /* required */,MAP_SHARED /* recommended */, cam->fd, buf.m.offset);
    
        if (MAP_FAILED == cam->buffers[cam->n_buffers].start)
            errno_exit("mmap");
    }
}
/*内存回收*/
static void exit_camera(struct camera *cam)
{
    unsigned int i;

    for (i = 0; i < cam->n_buffers; ++i)
        if (-1 == munmap(cam->buffers[i].start, cam->buffers[i].length))
            errno_exit("munmap");
    free(cam->buffers);
}

static void start_capturing(struct camera *cam)
{
    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < cam->n_buffers; ++i) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    }
        
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(cam->fd, VIDIOC_STREAMON, &type))
        errno_exit("VIDIOC_STREAMON");
}
static void stop_capturing(struct camera *cam)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(cam->fd, VIDIOC_STREAMOFF, &type))
		errno_exit("VIDIOC_STREAMOFF");
}
/*外界接口*/
void v4l2_init(struct camera *cam)
{
    cam->device_name   = strdup(CAM_DEVICE);
    cam->buffers       = NULL;
    cam->display_depth = 5;
    cam->support_fmt   = 0;//
    cam->status = 0;
    if( cam->width == 0 || cam->height == 0){
	    //set it by args
    	cam->width         = CAM_WIDGH;
    	cam->height        = CAM_HEIGHT;
    }
log_msg("camera use %dX%d\n",cam->width,cam->height);

    open_camera(cam);
    init_camera(cam);
    h264_compress_begin( cam->width, cam->height );
    start_capturing(cam);
}
void v4l2_exit(struct camera *cam)
{
    stop_capturing(cam);
    exit_camera(cam);
    close_camera(cam);
    h264_compress_end();
}
/*JPEG 和YUYV422、YUV420格式应该不一样读取函数应该由差别
* 以下为yuyv422 和yuv420采集方式
* */
int read_frame(struct camera *cam,char *buffer,int *len/*数据大小*/)
{
    fd_set fds;
    struct timeval tv;
    int r;
    struct v4l2_buffer buf;
    
    
    FD_ZERO(&fds);
    FD_SET(cam->fd,&fds);

    tv.tv_sec  = 2;
    tv.tv_usec = 0;

    r = select(cam->fd+1,&fds,NULL,NULL,&tv);
    if(-1 == r){
        if(EINTR == errno)
            return 0;//表示应该继续读
        return -1;
    }
    if(0 == r){
        fprintf(stderr,"select timeout");
        return 0;
    }
    
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(cam->fd, VIDIOC_DQBUF, &buf)) {
	    log_msg("read__frame : get camera buffer error : %s\n",strerror(errno));
	           return -1;
	}
    memcpy(buffer,(unsigned char *)cam->buffers[buf.index].start,buf.bytesused);
    //printf("cam->n_buffers=%d\nbuf.index=%d\nbuf.bytesused=%d\n",cam->n_buffers,buf.index,buf.bytesused);
    *len = buf.bytesused;
    if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf))
        log_msg("VIDIOC_QBUF error ");
    dbug("video QBUF");
    return 1;//成功
}

int read_encode_frame(struct camera *cam,char *buffer,int *len)
{
    fd_set fds;
    struct timeval tv;
    int r;
    struct v4l2_buffer buf;
    
    
    FD_ZERO(&fds);
    FD_SET(cam->fd,&fds);

    tv.tv_sec  = 3;
    tv.tv_usec = 0;//40ms

    r = select(cam->fd+1,&fds,NULL,NULL,&tv);
    if(-1 == r){
        if(EINTR == errno)
            return 0;//表示应该继续读
        return -1;
    }
    if(0 == r){
        fprintf(stderr,"select timeout\n");
        return 0;
    }
    
    CLEAR(buf);
    
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(cam->fd, VIDIOC_DQBUF, &buf)) {
	    log_msg("read_encode_frame : get camera buffer error : %s\n",strerror(errno));
            return -1;
    }
    r = 1;
#ifdef USE_X264_CODER
    if( is_h264_coder_ok() ){
	log_msg(" get a camera pic %d B and to compress\n",buf.bytesused);
    	*len = h264_compress_frame( -1, (char *)cam->buffers[buf.index].start ,buf.bytesused, buffer) ;
	log_msg(" compress over\n");
    	if( 0 >= *len ){
		*len = 0;
		r = 0;
	}
    }else{
	log_msg(" get a camera pic and to copy\n");
    	memcpy(buffer,(unsigned char *)cam->buffers[buf.index].start,buf.bytesused);
    }
#else
	log_msg(" get a camera pic %d B \n",buf.bytesused);
    memcpy(buffer,(unsigned char *)cam->buffers[buf.index].start,buf.bytesused);
    *len = buf.bytesused;
#endif

    //printf("cam->n_buffers=%d\nbuf.index=%d\nbuf.bytesused=%d\n",cam->n_buffers,buf.index,buf.bytesused);
    if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf))
        log_msg("VIDIOC_QBUF error , ignore?");

    return r;//成功
}
