#ifndef TOUCH_H
#define TOUCH_H

#include <linux/input.h>

// 打开触摸屏设备，返回文件描述符，失败返回-1
typedef struct touch_device
{
  int fd;
  const char *dev_path;
} touch_device_t;

// 打开触摸屏设备
touch_device_t *touch_open(const char *dev_path);
// 关闭触摸屏设备
void touch_close(touch_device_t *dev);
// 读取触摸屏事件，返回0成功，-1失败
int touch_read_event(touch_device_t *dev, struct input_event *event);

// 读取一次触摸屏的x和y坐标，成功返回0，失败返回-1
int touch_get_xy(touch_device_t *dev, int *x, int *y);

#endif // TOUCH_H
