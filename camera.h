#ifndef CAMERA_H
#define CAMERA_H

#include <linux/videodev2.h>

#define CAMERA_BUF_COUNT 4

// 默认分辨率宏定义（如需自定义可在.c文件中修改）
#define BMP_CAMERA_W 320
#define BMP_CAMERA_H 240

// YUV转ARGB
int yuvtoargb(int y, int u, int v);
// 一帧YUYV转ARGB
int allyuyvtoargb(char *yuyvdata, int *argbdata);

struct camerabuf
{
  void *start;
  int len;
};

struct camera
{
  int fd;
  int width;
  int height;
  struct camerabuf bufs[CAMERA_BUF_COUNT];
  struct v4l2_buffer vbuf;
};

// 打开并初始化摄像头，返回camera结构体指针，失败返回NULL
typedef struct camera camera_t;
camera_t *camera_open(const char *dev, int width, int height);
// 启动摄像头采集，成功返回0
int camera_start(camera_t *cam);
// 采集一帧，返回缓冲区指针和长度（通过参数），失败返回NULL
void *camera_capture(camera_t *cam, int *out_len, int *buf_index);
// 采集后重新入队
int camera_requeue(camera_t *cam, int buf_index);
// 停止采集
int camera_stop(camera_t *cam);
// 关闭摄像头并释放资源
void camera_close(camera_t *cam);

#endif // CAMERA_H
