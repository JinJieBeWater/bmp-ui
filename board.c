#include <stdio.h>
#include <unistd.h>
#include "touch.h"
#include "bmp.h"
#include "touch.h"

int main()
{
	// 显示登录图片
	if (bmp_show("./resources/login.bmp", 0, 0, 800, 480, "/dev/fb0") != 0)
	{
		printf("显示登录图片失败!\n");
		return -1;
	}

	touch_device_t *ts = touch_open("/dev/input/event0");
	if (!ts)
	{
		printf("打开触摸屏失败了!\n");
		return -1;
	}

	int x = -1, y = -1;
	printf("请触摸屏幕...\n");
	// 通用事件区域结构体
	typedef struct
	{
		const char *name;
		int x1, y1, x2, y2;
		void (*handler)(void); // 事件处理函数指针
	} touch_region_t;

	// 事件处理函数声明
	void on_login() { printf("触发事件: 登录按钮\n"); /* TODO: 登录事件处理 */ }
	void on_register() { printf("触发事件: 注册按钮\n"); /* TODO: 注册事件处理 */ }
	void on_welcome() { printf("触发事件: 欢迎版\n"); /* TODO: 欢迎版事件处理 */ }
	void on_avatar() { printf("触发事件: 头像\n"); /* TODO: 头像事件处理 */ }

	// 区域数组，后续可直接添加新区域
	touch_region_t regions[] = {
			{"登录按钮", 450, 250, 600, 300, on_login},
			{"注册按钮", 450, 350, 600, 400, on_register},
			{"欢迎版", 450, 100, 700, 200, on_welcome},
			{"头像", 50, 130, 370, 360, on_avatar},
	};
	int region_count = sizeof(regions) / sizeof(regions[0]);

	while (1)
	{
		if (touch_get_xy(ts, &x, &y) == 0)
		{
			printf("检测到触摸: x=%d, y=%d\n", x, y);
			int hit = 0;
			for (int i = 0; i < region_count; ++i)
			{
				if (x >= regions[i].x1 && x <= regions[i].x2 && y >= regions[i].y1 && y <= regions[i].y2)
				{
					if (regions[i].handler)
						regions[i].handler();
					hit = 1;
					break;
				}
			}
			if (!hit)
				printf("未命中任何事件区域\n");
		}
		usleep(1000); // 1ms
	}

	touch_close(ts);
	return 0;
}
