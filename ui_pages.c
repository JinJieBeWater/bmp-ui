#include "ui_pages.h"
#include "ui_router.h"
#include "bmp.h"
#include "config.h"
#include <stddef.h>

// 事件处理函数声明
void on_login() { /* TODO: 登录事件处理 */ }
void on_register() { /* TODO: 注册事件处理 */ }
void on_welcome() { /* TODO: 欢迎版事件处理 */ }
void on_avatar() { /* TODO: 头像事件处理 */ }

// 登录页面的触摸区域
static touch_region_t login_regions[] = {
    {"登录按钮", 450, 250, 600, 300, on_login},
    {"注册按钮", 450, 350, 600, 400, on_register},
    {"欢迎版", 450, 100, 700, 200, on_welcome},
    {"头像", 50, 130, 370, 360, on_avatar},
};
static int login_region_count = sizeof(login_regions) / sizeof(login_regions[0]);

// 欢迎页面的触摸区域
static touch_region_t welcome_regions[] = {
    {"登录按钮", 450, 250, 600, 300, on_login},
    {"注册按钮", 450, 350, 600, 400, on_register},
    {"欢迎版", 450, 100, 700, 200, on_welcome},
    {"头像", 50, 130, 370, 360, on_avatar},
};
static int welcome_region_count = sizeof(welcome_regions) / sizeof(welcome_regions[0]);

// bmp_show参数结构体
typedef struct
{
  const char *bmp_path;
  int lcd_x, lcd_y, lcd_width, lcd_height;
  const char *fb_path;
} bmp_show_param_t;

// 通用bmp_show回调
static void bmp_show_callback(void *param)
{
  bmp_show_param_t *p = (bmp_show_param_t *)param;
  bmp_show(p->bmp_path, p->lcd_x, p->lcd_y, p->lcd_width, p->lcd_height, p->fb_path);
}

void ui_pages_init(void)
{
  ui_router_init();
  static bmp_show_param_t login_param = {BMP_LOGIN_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH};
  static bmp_show_param_t welcome_param = {BMP_WELCOME_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH};
  ui_router_register("login", bmp_show_callback, &login_param, login_regions, login_region_count);
  ui_router_register("welcome", bmp_show_callback, &welcome_param, welcome_regions, welcome_region_count);
  ui_router_push("login", NULL);
}
