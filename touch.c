#include "touch.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

// 打开触摸屏设备
touch_device_t *touch_open(const char *dev_path)
{
  touch_device_t *dev = (touch_device_t *)malloc(sizeof(touch_device_t));
  if (!dev)
    return NULL;
  dev->dev_path = dev_path;
  dev->fd = open(dev_path, O_RDWR);
  if (dev->fd == -1)
  {
    free(dev);
    return NULL;
  }
  return dev;
}

// 关闭触摸屏设备
void touch_close(touch_device_t *dev)
{
  if (dev)
  {
    if (dev->fd != -1)
      close(dev->fd);
    free(dev);
  }
}

// 读取触摸屏事件
int touch_read_event(touch_device_t *dev, struct input_event *event)
{
  if (!dev || dev->fd == -1)
    return -1;
  int ret = read(dev->fd, event, sizeof(struct input_event));
  if (ret != sizeof(struct input_event))
    return -1;
  return 0;
}

// 读取一次触摸屏的x和y坐标（不缩放），成功返回0，失败返回-1
int touch_get_xy(touch_device_t *dev, int *x, int *y)
{
  struct input_event myinput;
  int got_x = 0, got_y = 0;
  int tx = -1, ty = -1;
  if (!dev || dev->fd == -1)
    return -1;
  while (1)
  {
    int ret = read(dev->fd, &myinput, sizeof(myinput));
    if (ret != sizeof(myinput))
      return -1;
    if (myinput.type == EV_ABS)
    {
      if (myinput.code == ABS_X)
      {
        tx = myinput.value;
        if (x)
          *x = tx;
        printf("x坐标是: %d\n", tx);
        got_x = 1;
      }
      if (myinput.code == ABS_Y)
      {
        ty = myinput.value;
        if (y)
          *y = ty;
        printf("y坐标是: %d\n", ty);
        got_y = 1;
      }
      if (got_x && got_y)
        break;
    }
  }
  return 0;
}
