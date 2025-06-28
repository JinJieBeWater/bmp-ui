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
