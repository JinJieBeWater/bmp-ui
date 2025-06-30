#ifndef __BMP_H__
#define __BMP_H__

int bmp_show(const char *bmp_path, int lcd_x, int lcd_y, int lcd_width, int lcd_height, const char *fb_path);
// 读取bmp指定区域到目标缓冲区
int bmp_read_region_to_buf(const char *bmp_path, int src_x, int src_y, int w, int h,
                           int *dst, int dst_stride, int dst_x, int dst_y);
// 合成多区域bmp到一块画布并显示
int bmp_compose_show(const char *header_bmp, const char *nav_bmp, const char *content_bmp, const char *fb_path);

#endif
