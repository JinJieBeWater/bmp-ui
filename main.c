
#include <stdio.h>
#include <unistd.h>
#include "touch.h"
#include "bmp.h"
#include "config.h"
#include "ui_router.h"
#include "ui_pages.h"

// 阶段2：打开触摸屏
touch_device_t *open_touch()
{
	touch_device_t *ts = touch_open(TOUCH_DEV_PATH);
	if (!ts)
	{
		printf("打开触摸屏失败了!\n");
		return NULL;
	}
	return ts;
}

// 通用事件循环，根据当前页面的触摸区域判断
void event_loop(touch_device_t *ts)
{
	int x = -1, y = -1;
	printf("请触摸屏幕...\n");
	while (1)
	{
		if (touch_get_xy(ts, &x, &y) == 0)
		{
			printf("检测到触摸: x=%d, y=%d\n", x, y);
			int hit = 0;
			const ui_page_t *cur = ui_router_current();
			if (cur && cur->regions && cur->region_count > 0)
			{
				touch_region_t *regions = (touch_region_t *)cur->regions;
				for (int i = 0; i < cur->region_count; ++i)
				{
					if (x >= regions[i].x1 && x <= regions[i].x2 && y >= regions[i].y1 && y <= regions[i].y2)
					{
						if (regions[i].handler)
							regions[i].handler();
						hit = 1;
						break;
					}
				}
			}
			if (!hit)
				printf("未命中任何事件区域\n");
		}
		usleep(1000); // 1ms
	}
}

// 开机动画播放函数
void play_boot_animation()
{
	char bmp_path[128];
	for (int i = 0; i <= 26; ++i)
	{
		snprintf(bmp_path, sizeof(bmp_path), "resources/boot/%d.bmp", i);
		bmp_show(bmp_path, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH);
		usleep(40000); // 40ms per frame, 可根据需要调整
	}
}

int main()
{
	play_boot_animation();
	ui_pages_init();
	touch_device_t *ts = open_touch();
	if (!ts)
		return -1;
	event_loop(ts);
	touch_close(ts);
	return 0;
}
