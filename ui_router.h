#ifndef UI_ROUTER_H
#define UI_ROUTER_H

#ifdef __cplusplus
extern "C"
{
#endif

  // 页面回调，参数可自定义
  typedef void (*ui_page_func_t)(void *param);

  // 页面结构体
  typedef struct ui_page
  {
    const char *name;
    ui_page_func_t on_show;
    ui_page_func_t on_hide; // 新增页面卸载回调
    void *param;
    void *regions; // 指向页面的触摸区域数组
    int region_count;
  } ui_page_t;

  // 注册页面
  int ui_router_register(const char *name, ui_page_func_t on_show, ui_page_func_t on_hide, void *param, void *regions, int region_count);
  // 跳转到页面
  int ui_router_push(const char *name, void *param);
  // 返回上一个页面
  int ui_router_pop();
  // 获取当前页面
  const ui_page_t *ui_router_current();
  // 初始化路由系统
  void ui_router_init();

#ifdef __cplusplus
}
#endif

#endif // UI_ROUTER_H
