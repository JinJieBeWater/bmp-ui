
#include <stdio.h>
#include <unistd.h>
#include "touch.h"
#include "bmp.h"
#include "config.h"
#include "ui_router.h"

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

// 登录页面的触摸区域
touch_region_t login_regions[] = {
		{"登录按钮", 450, 250, 600, 300, on_login},
		{"注册按钮", 450, 350, 600, 400, on_register},
		{"欢迎版", 450, 100, 700, 200, on_welcome},
		{"头像", 50, 130, 370, 360, on_avatar},
};
int login_region_count = sizeof(login_regions) / sizeof(login_regions[0]);

// 欢迎页面的触摸区域
touch_region_t welcome_regions[] = {};
int welcome_region_count = sizeof(welcome_regions) / sizeof(welcome_regions[0]);

void show_login_page(void *param)
{
	bmp_show(BMP_LOGIN_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH);
}
void show_welcome_page(void *param)
{
	bmp_show(BMP_WELCOME_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH); // 如无欢迎页图片可继续用登录页
}

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

int main()
{
	ui_router_init();
	// 注册页面及其触摸区域
	ui_router_register("login", show_login_page, NULL, login_regions, login_region_count);
	ui_router_register("welcome", show_welcome_page, NULL, welcome_regions, welcome_region_count);

	ui_router_push("login", NULL); // 启动时显示登录页

	touch_device_t *ts = open_touch();
	if (!ts)
		return -1;
	event_loop(ts);
	touch_close(ts);
	return 0;
}
