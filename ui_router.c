#include "ui_router.h"
#include <string.h>
#include <stdlib.h>

#define MAX_PAGES 16
#define MAX_STACK 8

static ui_page_t page_table[MAX_PAGES];
static int page_count = 0;

static const ui_page_t *page_stack[MAX_STACK];
static int stack_top = -1;

void ui_router_init()
{
  page_count = 0;
  stack_top = -1;
  memset(page_table, 0, sizeof(page_table));
  memset(page_stack, 0, sizeof(page_stack));
}

int ui_router_register(const char *name, ui_page_func_t on_show, void *param, void *regions, int region_count)
{
  if (page_count >= MAX_PAGES)
    return -1;
  page_table[page_count].name = name;
  page_table[page_count].on_show = on_show;
  page_table[page_count].param = param;
  page_table[page_count].regions = regions;
  page_table[page_count].region_count = region_count;
  page_count++;
  return 0;
}

static const ui_page_t *find_page(const char *name)
{
  for (int i = 0; i < page_count; ++i)
  {
    if (strcmp(page_table[i].name, name) == 0)
      return &page_table[i];
  }
  return NULL;
}

int ui_router_push(const char *name, void *param)
{
  if (stack_top + 1 >= MAX_STACK)
    return -1;
  const ui_page_t *page = find_page(name);
  if (!page)
    return -2;
  stack_top++;
  page_stack[stack_top] = page;
  if (page->on_show)
    page->on_show(param ? param : page->param);
  return 0;
}

int ui_router_pop()
{
  if (stack_top <= 0)
    return -1;
  stack_top--;
  const ui_page_t *page = page_stack[stack_top];
  if (page && page->on_show)
    page->on_show(page->param);
  return 0;
}

const ui_page_t *ui_router_current()
{
  if (stack_top < 0)
    return NULL;
  return page_stack[stack_top];
}
