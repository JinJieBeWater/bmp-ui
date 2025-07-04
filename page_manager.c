#include "page_manager.h"
#include "ui_router.h"
#include "page_login.h"
#include "page_dashboard.h"
#include "sqlite3.h"
#include <stdio.h>

static sqlite3 *g_db = NULL;

void page_manager_init(void)
{
    ui_router_init();

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

    page_login_init(g_db);
    page_dashboard_init();

    ui_router_push("login", NULL);
}
