#include "page_dashboard.h"
#include "common_ui.h"
#include "bmp.h"
#include "config.h"
#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

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
    // 关闭线程
    atomic_store(&g_temp_humidity_running, 0);
    pthread_exit(NULL);
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

  // 在这里修改IP和端口
  const char *server_ip = "192.168.17.85";
  int server_port = 6666;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("socket creation failed");
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
  {
    perror("invalid address");
    close(sockfd);
    return;
  }

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("connection failed");
    close(sockfd);
    return;
  }

  printf("成功连接到温湿度服务器 %s:%d\n", server_ip, server_port);

  atomic_store(&g_temp_humidity_running, 1);
  if (pthread_create(&g_temp_humidity_thread, NULL, temperature_humidity_thread, &sockfd) != 0)
  {
    perror("温湿度采集线程创建失败");
    atomic_store(&g_temp_humidity_running, 0);
    close(sockfd); // 线程创建失败，关闭socket
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

void page_dashboard_init(void)
{
  ui_router_register("dashboard", dashboard_show_callback, dashboard_hide_callback, NULL, dashboard_regions, dashboard_region_count);
}
