#include "bmp.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

static unsigned int bgr_to_argb(unsigned char b, unsigned char g, unsigned char r)
{
  return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

/**
 * @brief 在LCD帧缓冲区上显示24位BMP图片。
 *
 * 此函数从指定的文件路径读取24位BMP图片，并将其显示在LCD帧缓冲区的指定(lcd_x, lcd_y)位置。
 * 帧缓冲区被映射到内存中，BMP图片逐像素复制，并通过`bgr_to_argb`函数将每个像素从BGR格式转换为ARGB格式。
 *
 * @param bmp_path   BMP图片文件路径（必须为24位BMP）。
 * @param lcd_x      图片左上角在LCD上的X坐标。
 * @param lcd_y      图片左上角在LCD上的Y坐标。
 * @param lcd_width  LCD帧缓冲区的宽度（像素）。
 * @param lcd_height LCD帧缓冲区的高度（像素）。
 * @param fb_path    帧缓冲设备文件路径（如"/dev/fb0"）。
 *
 * @return 成功返回0，
 *         -1 无法打开BMP文件，
 *         -2 BMP不是24位格式，
 *         -3 无法打开帧缓冲设备，
 *         -4 帧缓冲区内存映射失败。
 *
 * @note 假设帧缓冲区为32位每像素（ARGB）。
 * @note BMP图片不会缩放，按原始大小绘制。
 * @note 本函数未对图片尺寸与LCD尺寸做越界检查。
 */
int bmp_show(const char *bmp_path, int lcd_x, int lcd_y, int lcd_width, int lcd_height, const char *fb_path)
{
  int bmpfd, lcdfd;
  int *lcdmem;
  int width, height;
  unsigned short bpp;
  unsigned int offset;
  unsigned char header[54];

  bmpfd = open(bmp_path, O_RDWR);
  if (bmpfd == -1)
    return -1;

  read(bmpfd, header, 54);
  width = *(int *)&header[18];
  height = *(int *)&header[22];
  bpp = *(unsigned short *)&header[28];
  offset = *(unsigned int *)&header[10];
  if (bpp != 24)
  {
    close(bmpfd);
    return -2;
  }

  lcdfd = open(fb_path, O_RDWR);
  if (lcdfd == -1)
  {
    close(bmpfd);
    return -3;
  }
  lcdmem = mmap(NULL, lcd_width * lcd_height * 4, PROT_READ | PROT_WRITE, MAP_SHARED, lcdfd, 0);
  if (lcdmem == MAP_FAILED)
  {
    close(bmpfd);
    close(lcdfd);
    return -4;
  }

  int line_bytes = ((width * 3 + 3) / 4) * 4;
  unsigned char *linebuf = malloc(line_bytes);
  int x, y;
  for (y = 0; y < height; y++)
  {
    lseek(bmpfd, offset + (height - 1 - y) * line_bytes, SEEK_SET);
    read(bmpfd, linebuf, line_bytes);
    for (x = 0; x < width; x++)
    {
      unsigned char b = linebuf[x * 3];
      unsigned char g = linebuf[x * 3 + 1];
      unsigned char r = linebuf[x * 3 + 2];
      int lcd_px = lcd_x + x;
      int lcd_py = lcd_y + y;
      if (lcd_px >= 0 && lcd_px < lcd_width && lcd_py >= 0 && lcd_py < lcd_height)
        lcdmem[lcd_py * lcd_width + lcd_px] = bgr_to_argb(b, g, r);
    }
  }
  free(linebuf);
  munmap(lcdmem, lcd_width * lcd_height * 4);
  close(lcdfd);
  close(bmpfd);
  return 0;
}

/**
 * @brief 从 BMP 文件中读取指定区域的数据到目标缓冲区
 *
 * 此函数从指定路径的 24 位 BMP 文件中，读取以 (src_x, src_y) 为左上角，宽为 w，高为 h 的矩形区域，
 * 并将其像素数据写入目标缓冲区 dst。目标缓冲区的每行跨度为 dst_stride，写入的起始点为 (dst_x, dst_y)。
 * 读取的像素会通过 bgr_to_argb 函数转换为 ARGB 格式。
 *
 * @param bmp_path   BMP 文件路径
 * @param src_x      源区域左上角 X 坐标（BMP 图像坐标系）
 * @param src_y      源区域左上角 Y 坐标（BMP 图像坐标系）
 * @param w          读取区域的宽度（像素）
 * @param h          读取区域的高度（像素）
 * @param dst        目标缓冲区指针（int 类型，每个像素一个 int）
 * @param dst_stride 目标缓冲区每行的跨度（int 数量）
 * @param dst_x      目标缓冲区写入起始点 X 坐标
 * @param dst_y      目标缓冲区写入起始点 Y 坐标
 * @return           0 表示成功，-1 表示文件打开失败，-2 表示 BMP 格式不支持（非 24 位）
 */
int bmp_read_region_to_buf(const char *bmp_path, int src_x, int src_y, int w, int h,
                           int *dst, int dst_stride, int dst_x, int dst_y)
{
  int bmpfd = open(bmp_path, O_RDWR);
  if (bmpfd == -1)
    return -1;
  unsigned char header[54];
  read(bmpfd, header, 54);
  int width = *(int *)&header[18];
  int height = *(int *)&header[22];
  unsigned short bpp = *(unsigned short *)&header[28];
  unsigned int offset = *(unsigned int *)&header[10];
  if (bpp != 24)
  {
    close(bmpfd);
    return -2;
  }
  int line_bytes = ((width * 3 + 3) / 4) * 4;
  unsigned char *linebuf = malloc(line_bytes);
  int y;
  for (y = 0; y < h; y++)
  {
    int bmp_y = src_y + y;
    if (bmp_y < 0 || bmp_y >= height)
      continue;
    lseek(bmpfd, offset + (height - 1 - bmp_y) * line_bytes, SEEK_SET);
    read(bmpfd, linebuf, line_bytes);
    int x;
    for (x = 0; x < w; x++)
    {
      int bmp_x = src_x + x;
      if (bmp_x < 0 || bmp_x >= width)
        continue;
      unsigned char b = linebuf[bmp_x * 3];
      unsigned char g = linebuf[bmp_x * 3 + 1];
      unsigned char r = linebuf[bmp_x * 3 + 2];
      int dst_px = dst_x + x;
      int dst_py = dst_y + y;
      dst[dst_py * dst_stride + dst_px] = bgr_to_argb(b, g, r);
    }
  }
  free(linebuf);
  close(bmpfd);
  return 0;
}

/**
 * @brief 合成并显示BMP图片到帧缓冲区
 *
 * 此函数将三个BMP图片（header、nav、content）按指定区域合成到一个800x480的画布上，
 * 并将合成结果写入指定的帧缓冲设备（fb_path）。
 *
 * 合成规则如下：
 * - header_bmp：放置于画布顶部，区域为(0,0,800,80)
 * - nav_bmp：放置于画布左侧，区域为(0,80,100,300)
 * - content_bmp：放置于画布剩余部分，区域为(100,80,700,400)
 *
 * @param header_bmp   头部BMP图片文件路径
 * @param nav_bmp      导航栏BMP图片文件路径
 * @param content_bmp  内容区BMP图片文件路径
 * @param fb_path      帧缓冲设备文件路径
 * @return int         0表示成功，负值表示失败（-10: 内存分配失败，-11: 打开帧缓冲失败，-12: 映射帧缓冲失败）
 */
int bmp_compose_show(const char *header_bmp, const char *nav_bmp, const char *content_bmp, const char *fb_path)
{
  int lcd_width = 800, lcd_height = 480;
  int *canvas = malloc(lcd_width * lcd_height * 4);
  if (!canvas)
    return -10;
  memset(canvas, 0xFF, lcd_width * lcd_height * 4); // 白底
  // header: x0=0,y0=0,w=800,h=80
  bmp_read_region_to_buf(header_bmp, 0, 0, 800, 80, canvas, lcd_width, 0, 0);
  // nav: x0=0,y0=80,w=100,h=300
  bmp_read_region_to_buf(nav_bmp, 0, 80, 100, 300, canvas, lcd_width, 0, 80);
  // content: x0=100,y0=80,w=700,h=400
  bmp_read_region_to_buf(content_bmp, 100, 80, 700, 400, canvas, lcd_width, 100, 80);
  // 写入fb
  int lcdfd = open(fb_path, O_RDWR);
  if (lcdfd == -1)
  {
    free(canvas);
    return -11;
  }
  int *lcdmem = mmap(NULL, lcd_width * lcd_height * 4, PROT_READ | PROT_WRITE, MAP_SHARED, lcdfd, 0);
  if (lcdmem == MAP_FAILED)
  {
    close(lcdfd);
    free(canvas);
    return -12;
  }
  memcpy(lcdmem, canvas, lcd_width * lcd_height * 4);
  munmap(lcdmem, lcd_width * lcd_height * 4);
  close(lcdfd);
  free(canvas);
  return 0;
}
