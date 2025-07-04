#include "ui_pages.h"
#include "ui_router.h"
#include "bmp.h"
#include "config.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include <string.h>
#include "camera.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

static sqlite3 *g_db = NULL;

// 登录事件处理
void on_login()
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
void on_register()
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

// dashboard
#include <stdatomic.h>

// 摄像头全局指针
static camera_t *g_camera = NULL;
static atomic_int g_camera_running = ATOMIC_VAR_INIT(0);
static pthread_t g_camera_thread;

// 线程采集函数
static void *camera_capture_thread(void *arg)
{
  while (atomic_load(&g_camera_running))
  {
    int frame_len, buf_index;
    void *frame = camera_capture(g_camera, &frame_len, &buf_index);
    if (!frame)
    {
      printf("采集摄像头帧失败！\n");
      break;
    }
    static int argbbuf[BMP_CAMERA_W * BMP_CAMERA_H];
    allyuyvtoargb((char *)frame, argbbuf);
    int lcdfd = open(LCD_FB_PATH, O_RDWR);
    if (lcdfd == -1)
    {
      printf("打开LCD失败！\n");
      camera_requeue(g_camera, buf_index);
      break;
    }
    int *lcdmem = (int *)mmap(NULL, LCD_WIDTH * LCD_HEIGHT * 4, PROT_READ | PROT_WRITE, MAP_SHARED, lcdfd, 0);
    if (lcdmem == MAP_FAILED)
    {
      printf("LCD映射失败！\n");
      close(lcdfd);
      camera_requeue(g_camera, buf_index);
      break;
    }
    int x1 = 110, y1 = 100, x2 = 430, y2 = 340;
    int w = x2 - x1;
    int h = y2 - y1;
    for (int j = 0; j < h; j++)
    {
      memcpy(lcdmem + (y1 + j) * LCD_WIDTH + x1, &argbbuf[j * BMP_CAMERA_W], w * 4);
    }
    munmap(lcdmem, LCD_WIDTH * LCD_HEIGHT * 4);
    close(lcdfd);
    camera_requeue(g_camera, buf_index);
    usleep(33000); // 约30fps
  }
  printf("摄像头采集线程退出\n");
  return NULL;
}

void on_open_camera()
{
  if (g_camera_running)
  {
    printf("摄像头采集已在运行\n");
    return;
  }
  g_camera = camera_open("/dev/video7", BMP_CAMERA_W, BMP_CAMERA_H);
  if (!g_camera)
  {
    printf("摄像头打开失败！\n");
    return;
  }
  if (camera_start(g_camera) == -1)
  {
    printf("摄像头启动失败！\n");
    camera_close(g_camera);
    g_camera = NULL;
    return;
  }
  atomic_store(&g_camera_running, 1);
  if (pthread_create(&g_camera_thread, NULL, camera_capture_thread, NULL) != 0)
  {
    printf("采集线程创建失败！\n");
    g_camera_running = 0;
    camera_stop(g_camera);
    camera_close(g_camera);
    g_camera = NULL;
    return;
  }
  printf("摄像头已打开并启动，采集线程运行中...\n");
}

void on_close_camera()
{
  if (!g_camera_running)
  {
    printf("摄像头未打开\n");
    return;
  }
  atomic_store(&g_camera_running, 0);
  pthread_join(g_camera_thread, NULL); // 等待采集线程退出
  camera_stop(g_camera);
  camera_close(g_camera);
  g_camera = NULL;
  printf("摄像头已关闭\n");
}

void on_logout()
{
  printf("退出登录事件触发\n");
}

static atomic_int g_temp_humidity_running = ATOMIC_VAR_INIT(0);
static pthread_t g_temp_humidity_thread;

// 温湿度采集线程
static void *temperature_humidity_thread(void *arg)
{
  // 在这里修改IP和端口
  const char *server_ip = "192.168.17.85";
  int server_port = 6666;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("socket creation failed");
    return NULL;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
  {
    perror("invalid address");
    close(sockfd);
    return NULL;
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)

  {
    perror("connection failed");
    close(sockfd);
    return NULL;
  }

  printf("成功连接到温湿度服务器 %s:%d\n", server_ip, server_port);
  srand(time(NULL));

  while (atomic_load(&g_temp_humidity_running))
  {
    // 生成随机温湿度数据
    float temperature = 20.0 + (rand() / (float)RAND_MAX) * 10.0; // 20.0 - 30.0
    float humidity = 50.0 + (rand() / (float)RAND_MAX) * 20.0;    // 50.0 - 70.0

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"temperature\":%.1f,\"humidity\":%.1f}", temperature, humidity);

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
      perror("send failed");
      break;
    }
    printf("已发送数据: %s\n", buffer);

    // 在这里修改发送间隔（秒）
    sleep(5);
  }

  close(sockfd);
  printf("温湿度采集线程退出\n");
  return NULL;
}

void on_start_capture_temperature_humidity()
{
  if (atomic_load(&g_temp_humidity_running))
  {
    printf("温湿度采集已在运行\n");
    return;
  }

  atomic_store(&g_temp_humidity_running, 1);
  if (pthread_create(&g_temp_humidity_thread, NULL, temperature_humidity_thread, NULL) != 0)
  {
    perror("温湿度采集线程创建失败");
    atomic_store(&g_temp_humidity_running, 0);
  }
  else
  {
    printf("温湿度采集线程已启动\n");
  }
}

void on_stop_capture_temperature_humidity()
{
  if (!atomic_load(&g_temp_humidity_running))
  {
    printf("温湿度采集未运行\n");
    return;
  }
  atomic_store(&g_temp_humidity_running, 0);
  pthread_join(g_temp_humidity_thread, NULL); // 等待线程退出
  printf("温湿度采集已停止\n");
}

static touch_region_t dashboard_regions[] = {
    {"打开摄像头", 110, 360, 250, 390, on_open_camera},
    {"关闭摄像头", 290, 360, 430, 390, on_close_camera},
    {"退出登录", 700, 20, 780, 50, on_logout},
    {"开始采集温湿度", 570, 100, 670, 130, on_start_capture_temperature_humidity},
    {"停止采集温湿度", 690, 100, 790, 130, on_stop_capture_temperature_humidity}};

static int dashboard_region_count = sizeof(dashboard_regions) / sizeof(dashboard_regions[0]);

// dashboard页面卸载回调
static void dashboard_hide_callback(void *param)
{
  on_close_camera();                      // 确保关闭摄像头
  on_stop_capture_temperature_humidity(); // 确保停止采集温湿度
}

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

// 欢迎页显示回调，合成布局
static void dashboard_show_callback(void *param)
{
  // 可根据 param 传递内容做个性化扩展
  const char *header_bmp = "./resources/dashboard_header.bmp";
  const char *nav_bmp = "./resources/dashboard_nav.bmp";
  const char *content_bmp = "./resources/dashboard_home.bmp";
  bmp_compose_show(header_bmp, nav_bmp, content_bmp, LCD_FB_PATH);
  printf("已显示组合布局界面\n");
}

void ui_pages_init(void)
{
  ui_router_init();
  static bmp_show_param_t login_param = {BMP_LOGIN_PATH, 0, 0, LCD_WIDTH, LCD_HEIGHT, LCD_FB_PATH};
  ui_router_register("login", bmp_show_callback, NULL, &login_param, login_regions, login_region_count);
  ui_router_register("dashboard", dashboard_show_callback, dashboard_hide_callback, NULL, dashboard_regions, dashboard_region_count);

  // 打开数据库并创建用户表
  int ret = sqlite3_open("./new.db", &g_db);
  if (ret != SQLITE_OK)
  {
    printf("无法打开数据库！\n");
    g_db = NULL;
  }
  else
  {
    char *errmsg = NULL;
    ret = sqlite3_exec(g_db, "create table if not exists usertable (username text primary key, password text);", NULL, NULL, &errmsg);
    if (ret != SQLITE_OK)
    {
      printf("创建用户表失败: %s\n", errmsg);
      sqlite3_free(errmsg);
    }
  }

  ui_router_push("login", NULL);
}
