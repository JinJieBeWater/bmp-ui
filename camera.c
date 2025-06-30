#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

// 封装函数把一组YUV转成ARGB
int yuvtoargb(int y, int u, int v)
{
  int r, g, b;
  r = y + 1.4075 * (v - 128);
  g = y - 0.3455 * (u - 128) - 0.7169 * (v - 128);
  b = y + 1.779 * (u - 128);

  // 限制RGB分量在0~255
  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;
  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;
  if (b < 0)
    b = 0;
  if (b > 255)
    b = 255;

  // ARGB格式，A=0xFF
  int pix = (0xFF << 24) | (r << 16) | (g << 8) | b;
  return pix;
}

// 封装函数把一帧画面所有的YUYV转成ARGB
/*
  yuyvdata --》一帧画面YUYV数据首地址
  argbdata --》存放转换得到的一帧画面所有的ARGB数据
*/
int allyuyvtoargb(char *yuyvdata, int *argbdata)
{
  int i, j;
  for (i = 0, j = 0; i < (BMP_CAMERA_W * BMP_CAMERA_H) / 2; i++, j++) // 总的像素点W*H个
  {
    // Y1跟UV配合--》得到ARGB
    argbdata[2 * i] = yuvtoargb(yuyvdata[4 * j], yuyvdata[4 * j + 1], yuyvdata[4 * j + 3]);
    // Y2跟UV配合--》得到ARGB
    argbdata[2 * i + 1] = yuvtoargb(yuyvdata[4 * j + 2], yuyvdata[4 * j + 1], yuyvdata[4 * j + 3]);
  }
  return 0;
}

// 打开并初始化摄像头，返回camera结构体指针，失败返回NULL
camera_t *camera_open(const char *dev, int width, int height)
{
  // 分配camera结构体
  camera_t *cam = calloc(1, sizeof(camera_t));
  if (!cam)
    return NULL;
  // 打开摄像头设备
  cam->fd = open(dev, O_RDWR);
  if (cam->fd == -1)
  {
    free(cam);
    return NULL;
  }
  cam->width = width;
  cam->height = height;
  // 设置采集格式
  struct v4l2_format vfmt;
  memset(&vfmt, 0, sizeof(vfmt));
  vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vfmt.fmt.pix.width = width;
  vfmt.fmt.pix.height = height;
  vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  if (ioctl(cam->fd, VIDIOC_S_FMT, &vfmt) == -1)
  {
    close(cam->fd);
    free(cam);
    return NULL;
  }
  // 申请缓冲区
  struct v4l2_requestbuffers reqbuf;
  memset(&reqbuf, 0, sizeof(reqbuf));
  reqbuf.count = CAMERA_BUF_COUNT;
  reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  reqbuf.memory = V4L2_MEMORY_MMAP;
  if (ioctl(cam->fd, VIDIOC_REQBUFS, &reqbuf) == -1)
  {
    close(cam->fd);
    free(cam);
    return NULL;
  }
  // 分配缓冲块并映射
  for (int i = 0; i < CAMERA_BUF_COUNT; i++)
  {
    struct v4l2_buffer vbuf;
    memset(&vbuf, 0, sizeof(vbuf));
    vbuf.index = i;
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(cam->fd, VIDIOC_QUERYBUF, &vbuf) == -1)
    {
      camera_close(cam);
      return NULL;
    }
    cam->bufs[i].len = vbuf.length;
    cam->bufs[i].start = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, vbuf.m.offset);
    if (cam->bufs[i].start == MAP_FAILED)
    {
      camera_close(cam);
      return NULL;
    }
    // 入队
    if (ioctl(cam->fd, VIDIOC_QBUF, &vbuf) == -1)
    {
      camera_close(cam);
      return NULL;
    }
  }
  return cam;
}

// 启动摄像头采集
int camera_start(camera_t *cam)
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  return ioctl(cam->fd, VIDIOC_STREAMON, &type);
}

// 采集一帧，返回缓冲区指针和长度（通过参数），失败返回NULL
void *camera_capture(camera_t *cam, int *out_len, int *buf_index)
{
  struct v4l2_buffer *vbuf = &cam->vbuf;
  memset(vbuf, 0, sizeof(*vbuf));
  vbuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf->memory = V4L2_MEMORY_MMAP;
  if (ioctl(cam->fd, VIDIOC_DQBUF, vbuf) == -1)
    return NULL;
  if (out_len)
    *out_len = vbuf->bytesused;
  if (buf_index)
    *buf_index = vbuf->index;
  return cam->bufs[vbuf->index].start;
}

// 采集后重新入队
int camera_requeue(camera_t *cam, int buf_index)
{
  struct v4l2_buffer vbuf;
  memset(&vbuf, 0, sizeof(vbuf));
  vbuf.index = buf_index;
  vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vbuf.memory = V4L2_MEMORY_MMAP;
  return ioctl(cam->fd, VIDIOC_QBUF, &vbuf);
}

// 停止采集
int camera_stop(camera_t *cam)
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  return ioctl(cam->fd, VIDIOC_STREAMOFF, &type);
}

// 关闭摄像头并释放资源
void camera_close(camera_t *cam)
{
  if (!cam)
    return;
  for (int i = 0; i < CAMERA_BUF_COUNT; i++)
  {
    if (cam->bufs[i].start && cam->bufs[i].len > 0)
      munmap(cam->bufs[i].start, cam->bufs[i].len);
  }
  if (cam->fd != -1)
    close(cam->fd);
  free(cam);
}
