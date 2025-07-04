#ifndef COMMON_UI_H
#define COMMON_UI_H

// 通用事件区域结构体
typedef struct
{
  const char *name;
  int x1, y1, x2, y2;
  void (*handler)(void);
} touch_region_t;

#endif // COMMON_UI_H
