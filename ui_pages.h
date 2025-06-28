#ifndef UI_PAGES_H
#define UI_PAGES_H

// 通用事件区域结构体
typedef struct
{
  const char *name;
  int x1, y1, x2, y2;
  void (*handler)(void);
} touch_region_t;

void ui_pages_init(void);

#endif // UI_PAGES_H
