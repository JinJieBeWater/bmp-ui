
#include <stdio.h>
#include <unistd.h>
#include "touch.h"
#include "bmp.h"
#include "config.h"
#include "ui_router.h"
#include "ui_pages.h"

/**
 * @brief 打开触摸设备并返回设备句柄指针。
 *
 * 此函数尝试打开指定路径的触摸设备，如果打开失败则打印错误信息并返回NULL。
 *
 * @return touch_device_t* 成功时返回触摸设备句柄指针，失败时返回NULL。
 */
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

/**
 * @brief 事件循环，持续检测触摸屏幕的输入并分发事件。
 *
 * 此函数会不断轮询触摸设备，获取当前的触摸坐标 (x, y)。
 * 当检测到有效的触摸输入时，会遍历当前页面的所有触摸区域（regions），
 * 判断触摸点是否落在某个区域内。如果命中区域且该区域有事件处理函数（handler），
 * 则调用对应的处理函数。若未命中任何区域，则输出提示信息。
 *
 * @param ts 指向触摸设备的指针，用于获取触摸坐标。
 */
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

/**
 * @brief 播放开机动画。
 *
 * 此函数依次加载并显示位于 "resources/boot/" 目录下的 0.bmp 到 26.bmp 共27帧 BMP 图片，
 * 每帧显示40毫秒，实现开机动画效果。图片将被显示在整个LCD屏幕上。
 */
void play_boot_animation()
{
	char bmp_path[128];
	for (int i = 0; i <= 26; ++i)
	{
		snprintf(bmp_path, sizeof(bmp_path), "resources/boot/%d.bmp", i);
		bmp_show(bmp_path, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH);
		usleep(40000);
	}
}

/**
 * @brief 程序主入口函数。
 *
 * 此函数依次执行以下操作：
 * 1. 播放启动动画。
 * 2. 初始化 UI 页面。
 * 3. 打开触摸设备，获取触摸设备句柄。
 * 4. 如果触摸设备打开失败，返回 -1。
 * 5. 进入事件循环，处理触摸事件。
 * 6. 关闭触摸设备。
 * 7. 返回 0，表示程序正常结束。
 *
 * @return int 返回 0 表示成功，返回 -1 表示触摸设备打开失败。
 */
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
