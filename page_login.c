#include "page_login.h"
#include "common_ui.h"
#include "bmp.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static sqlite3 *g_db = NULL;

// 登录事件处理
static void on_login()
{
  char username[32], password[32];
  printf("请输入用户名: ");
  scanf("%31s", username);
  printf("请输入密码: ");
  scanf("%31s", password);

  // 查询用户
  const char *sql = "select * from usertable where username=? and password=?;";
  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK)
  {
    printf("数据库查询失败！\n");
    return;
  }
  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);
  ret = sqlite3_step(stmt);
  if (ret == SQLITE_ROW)
  {
    printf("登录成功！\n");
    ui_router_push("dashboard", NULL);
  }
  else
  {
    printf("用户名或密码错误！\n");
  }
  sqlite3_finalize(stmt);
}

// 注册事件处理
static void on_register()
{
  char username[32], password[32];
  printf("请输入新用户名: ");
  scanf("%31s", username);
  printf("请输入新密码: ");
  scanf("%31s", password);

  // 检查用户名是否已存在
  const char *check_sql = "select * from usertable where username=?;";
  sqlite3_stmt *stmt = NULL;
  int ret = sqlite3_prepare_v2(g_db, check_sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK)
  {
    printf("数据库查询失败！\n");
    return;
  }
  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  ret = sqlite3_step(stmt);
  if (ret == SQLITE_ROW)
  {
    printf("用户名已存在！\n");
    sqlite3_finalize(stmt);
    return;
  }
  sqlite3_finalize(stmt);

  // 插入新用户
  const char *insert_sql = "insert into usertable (username, password) values (?, ?);";
  ret = sqlite3_prepare_v2(g_db, insert_sql, -1, &stmt, NULL);
  if (ret != SQLITE_OK)
  {
    printf("数据库插入失败！\n");
    return;
  }
  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);
  ret = sqlite3_step(stmt);
  if (ret == SQLITE_DONE)
  {
    printf("注册成功！\n");
    ui_router_push("login", NULL);
  }
  else
  {
    printf("注册失败！\n");
  }
  sqlite3_finalize(stmt);
}

// 登录页面的触摸区域
static touch_region_t login_regions[] = {
    {"登录按钮", 450, 250, 600, 300, on_login},
    {"注册按钮", 450, 350, 600, 400, on_register},
};
static int login_region_count = sizeof(login_regions) / sizeof(login_regions[0]);

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

void page_login_init(sqlite3 *db)
{
    g_db = db;
    static bmp_show_param_t login_param = {BMP_LOGIN_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH};
    ui_router_register("login", bmp_show_callback, NULL, &login_param, login_regions, login_region_count);
}
