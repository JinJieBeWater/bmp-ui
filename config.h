#ifndef CONFIG_H
#define CONFIG_H

#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define LCD_FB_PATH "/dev/fb0"
#define TOUCH_DEV_PATH "/dev/input/event0"

#define BMP_LOGIN_PATH "./resources/login.bmp"
#define BMP_WELCOME_PATH "./resources/welcome.bmp" // 如有欢迎页图片

#define TEMP_HUMIDITY_SERVER_IP "192.168.14.49"
#define TEMP_HUMIDITY_SERVER_PORT 6666


#endif // CONFIG_H
