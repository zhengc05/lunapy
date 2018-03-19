#include <stdio.h>
#include "mem.h"
#include "tokenize.h"
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>

struct Token* new_token(struct CompileState *c, int col, int row, int type, int vs)
{
	struct Token* t;
	struct TokenPool *p;
	if (!c->token_pool)
	{
		c->token_pool = (struct TokenPool*)malloc(sizeof(struct TokenPool));
		c->token_pool->null_idx = 0;
		c->token_pool->next = 0;
	}

	p = c->token_pool;
	while (p->next)
	{
		p = p->next;
	}

	if (p->null_idx < 128)
	{
		int i = p->null_idx;
		p->null_idx ++;
		t = &(p->elem[i]);
		t->col = col;
		t->row = row;
		t->type = type;
		t->vs = vs;
		t->items.head = 0;
		t->items.num = 0;
		t->items.tail = 0;
		return t;
	}
	else
	{
		p->next = (struct TokenPool*)malloc(sizeof(struct TokenPool));
		p->next->null_idx = 1;
		p->next->next = 0;
		t = &(p->next->elem[0]);
		t->col = col;
		t->row = row;
		t->type = type;
		t->vs = vs;
		t->items.head = 0;
		t->items.num = 0;
		t->items.tail = 0;
		return t;
	}
}

struct TListItem* new_list_item(struct CompileState *c, struct Token* t)
{
	struct ListPool *p;
	if (!c->list_pool)
	{
		c->list_pool = (struct ListPool*)malloc(sizeof(struct ListPool));
		c->list_pool->null_idx = 0;
		c->list_pool->next = 0;
	}

	p = c->list_pool;
	while (p->next)
	{
		p = p->next;
	}

	if (p->null_idx < 128)
	{
		int i = p->null_idx;
		p->null_idx ++;
		p->elem[i].t = t;
		p->elem[i].next = 0;
		return &(p->elem[i]);
	}
	else
	{
		p->next = (struct ListPool*)malloc(sizeof(struct ListPool));
		p->next->null_idx = 1;
		p->next->next = 0;
		p->next->elem[0].t = t;
		p->next->elem[0].next = 0;
		return &(p->next->elem[0]);
	}
}

void append_list_item(struct CompileState *c, struct TList*list, struct Token* t)
{
	struct TListItem* item = new_list_item(c, t);
	if (!list->head)
	{
		list->head = item;
		list->tail = item;
		list->num = 1;
		return;
	}
	if (!list->tail)
	{
		list->tail = item;
	}
	else {
		list->tail->next = item;
	}
	list->tail = item;
	list->num++;
}

void move_list_items(struct TList*dstlist, struct TList*srclist, int st)
{
	struct TListItem* item = srclist->head, *prev = 0;
	while (st)
	{
		prev = item;
		item = item->next;
		st--;
	}
	dstlist->head = item;
	dstlist->tail = srclist->tail;
	dstlist->num = srclist->num - st;
	if (prev)
	{
		prev->next = 0;
		srclist->tail = prev;
	}
	else
	{
		srclist->head = 0;
		srclist->tail = 0;
	}
	srclist->num = st;
}

int get_list_len(struct TList* list)
{
	return list->num;
}

struct IntListItem* new_intList_item(struct CompileState *c, int v)
{
	struct IntListPool *p;
	if (!c->intList_pool)
	{
		c->intList_pool = (struct IntListPool*)malloc(sizeof(struct IntListPool));
		c->intList_pool->null_idx = 0;
		c->intList_pool->next = 0;
	}

	p = c->intList_pool;
	while (p->next)
	{
		p = p->next;
	}

	if (p->null_idx < 128)
	{
		int i = p->null_idx;
		p->null_idx ++;
		p->elem[i].v = v;
		p->elem[i].next = NULL;
		return &(p->elem[i]);
	}
	else
	{
		p->next = (struct IntListPool*)malloc(sizeof(struct IntListPool));
		p->next->null_idx = 1;
		p->next->next = 0;
		p->next->elem[0].v = v;
		p->next->elem[0].next = NULL;
		return &(p->next->elem[0]);
	}
}

void append_intList_item(struct CompileState *c, struct IntList*list, int v)
{
	struct IntListItem* item = new_intList_item(c, v);
	if (!list->head)
	{
		list->head = item;
		list->tail = item;
		list->num = 1;
		return;
	}
	if (!list->tail)
	{
		list->tail = item;
	}
	else {
		list->tail->next = item;
	}
	list->tail = item;
	list->num++;
}

struct IntListItem* find_int_item(struct IntList* lst, int v)
{
	struct IntListItem*p = lst->head;
	while (p)
	{
		if (p->v == v)
			return p;
		p = p->next;
	}
	return 0;
}

char *new_string(struct CompileState *c, int size)
{
	struct StringPool *p;
	int total_size = size;
	if (!c->string_pool)
	{
		if (total_size < 1024) total_size = 1024;
		else if (total_size < 2048) total_size = 2048;
		else if (total_size < 4096) total_size = 4096;
		c->string_pool = (struct StringPool*)malloc(sizeof(struct StringPool) + total_size);
		c->string_pool->null_idx = 0;
		c->string_pool->next = 0;
		c->string_pool->total_size = total_size;
	}

	p = c->string_pool;
	do
	{
		if ((p->null_idx + size) < p->total_size)
		{
			int i = p->null_idx;
			p->null_idx += size;
			return &(p->mem[i]);
		}
		if (!p->next) break;
		p = p->next;
	} while(p);

	if (total_size < 1024) total_size = 1024;
	else if (total_size < 2048) total_size = 2048;
	else if (total_size < 4096) total_size = 4096;
	p->next = (struct StringPool*)malloc(sizeof(struct StringPool) + total_size);
	p->next->null_idx = size;
	p->next->next = 0;
	p->next->total_size = total_size;
	return &(p->next->mem[0]);
}

struct StrListItem* new_strList_item(struct CompileState *c, const char* s)
{
	struct StrListPool *p;
	if (!c->strList_pool)
	{
		c->strList_pool = (struct StrListPool*)malloc(sizeof(struct StrListPool));
		c->strList_pool->null_idx = 0;
		c->strList_pool->next = 0;
	}

	p = c->strList_pool;
	while (p->next)
	{
		p = p->next;
	}

	if (p->null_idx < 128)
	{
		int i = p->null_idx;
		p->null_idx ++;
		p->elem[i].s = s;
		p->elem[i].next = 0;
		return &(p->elem[i]);
	}
	else
	{
		p->next = (struct StrListPool*)malloc(sizeof(struct StrListPool));
		p->next->null_idx = 1;
		p->next->next = 0;
		p->next->elem[0].s = s;
		p->next->elem[0].next = 0;
		return &(p->next->elem[0]);
	}
}

void append_strList_item(struct CompileState *c, struct StrList*list, const char* s)
{
	struct StrListItem* item = new_strList_item(c, s);
	if (!list->head)
	{
		list->head = item;
		list->tail = item;
		list->num = 1;
		return;
	}
	if (!list->tail)
	{
		list->tail = item;
	}
	else {
		list->tail->next = item;
	}
	list->tail = item;
	list->num++;
}

struct StrListItem* find_str_item(struct StrList* lst, char* s)
{
	struct StrListItem*p = lst->head;
	while (p)
	{
		if (!strcmp(p->s, s))
			return p;
		p = p->next;
	}
	return 0;
}

void u_error(struct CompileState *c, const char* ctx, const char*s, int col, int row)
{
	c->error_buffer = new_string(c, 512);
	snprintf(c->error_buffer, 512, "%s: (%d, %d)", ctx, col, row);
	longjmp(c->error_jmp_buf, 1);
}

const char* get_compile_error(struct CompileState *c)
{
	return c->error_buffer;
}

void clear_compile_mem(struct CompileState *c)
{
	while (c->token_pool)
	{
		struct TokenPool* p = c->token_pool;
		c->token_pool = c->token_pool->next;
		free(p);
	}

	while (c->list_pool)
	{
		struct ListPool* p = c->list_pool;
		c->list_pool = c->list_pool->next;
		free(p);
	}

	while (c->intList_pool)
	{
		struct IntListPool* p = c->intList_pool;
		c->intList_pool = c->intList_pool->next;
		free(p);
	}

	while (c->string_pool)
	{
		struct StringPool* p = c->string_pool;
		c->string_pool = c->string_pool->next;
		free(p);
	}

	while (c->strList_pool)
	{
		struct StrListPool* p = c->strList_pool;
		c->strList_pool = c->strList_pool->next;
		free(p);
	}
}

struct Item* new_code_item(struct CompileState *c, int isdata)
{
	struct Item* t;
	struct ItemPool *p;
	if (!c->item_pool)
	{
		c->item_pool = (struct ItemPool*)malloc(sizeof(struct ItemPool));
		c->item_pool->null_idx = 0;
		c->item_pool->next = 0;
	}

	p = c->item_pool;
	while (p->next)
	{
		p = p->next;
	}

	if (p->null_idx < 128)
	{
		int i = p->null_idx;
		p->null_idx++;
		t = &(p->elem[i]);
		t->isdata = isdata;
		t->next = 0;
		return t;
	}
	else
	{
		p->next = (struct ItemPool*)malloc(sizeof(struct ItemPool));
		p->next->null_idx = 1;
		p->next->next = 0;
		t = &(p->next->elem[0]);
		t->isdata = isdata;
		t->next = 0;
		return t;
	}
}

void append_itemList_item(struct ItemList*list, struct Item* item)
{
	item->next = 0;
	if (!list->head)
	{
		list->head = item;
		list->tail = item;
		list->num = 1;
		return;
	}
	if (!list->tail)
	{
		list->tail = item;
	}
	else {
		list->tail->next = item;
	}
	list->tail = item;
	list->num++;
}

void remove_itemList_item(struct ItemList*list, struct Item* item)
{
	/* TODO */
}

