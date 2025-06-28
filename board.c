
#include <stdio.h>

#include "touch.h"

int main()
{
	struct input_event myinput;
	touch_device_t *ts = touch_open("/dev/input/event0");
	if (!ts)
	{
		printf("打开触摸屏失败了!\n");
		return -1;
	}

	int x = -1, y = -1;
	while (1)
	{
		int ret = touch_read_event(ts, &myinput);
		if (ret == 0)
		{
			if (myinput.type == EV_ABS)
			{
				if (myinput.code == ABS_X)
				{
					x = myinput.value;
					printf("x坐标是: %d\n", x);
				}
				if (myinput.code == ABS_Y)
				{
					y = myinput.value;
					printf("y坐标是: %d\n", y);
				}
			}
			if (x != -1 && y != -1) // 已经读取到x和y
				break;
		}
		else if (ret == 1)
		{
			// 非阻塞，无事件，稍作等待
			usleep(1000); // 1ms
		}
		else
		{
			printf("读取触摸屏事件失败!\n");
			break;
		}
	}

	touch_close(ts);
	return 0;
}
}
