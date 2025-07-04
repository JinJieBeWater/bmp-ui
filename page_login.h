#ifndef PAGE_LOGIN_H
#define PAGE_LOGIN_H

#include "ui_router.h"
#include "sqlite3.h"

// 初始化并注册登录页面
void page_login_init(sqlite3 *db);

#endif // PAGE_LOGIN_H
