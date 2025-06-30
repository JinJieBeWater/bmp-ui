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
