

#include <stdio.h>
#include "touch.h"
#include "bmp.h"

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
	while (1)
	{
		if (touch_get_xy(ts, &x, &y) == 0)
		{
			printf("检测到触摸: x=%d, y=%d\n", x, y);
			// 这里可以根据需要处理坐标
		}
		usleep(1000); // 1ms
	}

	touch_close(ts);
	return 0;
}
}
