
#include "lp.h"
#include "lp_internal.h"

/**/

lp_obj* lp_none(LP)
{
	RETURN_LP_OBJ(lp->lp_None);
}

static struct LpObjPool * obj_pool_new()
{
	struct LpObjPool * pool = (struct LpObjPool *)malloc(sizeof(struct LpObjPool));
	memset(pool, 0, sizeof(struct LpObjPool));
	for (int i = 0; i < OBJ_SIZE; i++)
	{
		pool->obj[i].pool = pool;
		if (i == 0)
		{
			pool->obj[i].prev = 0;
			pool->obj[i].next = &pool->obj[i+1];
		}
		else if (i == OBJ_SIZE - 1)
		{
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = 0;
		}
		else {
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = &pool->obj[i + 1];
		}
	}
	pool->head = &pool->obj[0];
	pool->tail = &pool->obj[OBJ_SIZE-1];
	return pool;
}

static struct LpDictPool * dict_pool_new()
{
	struct LpDictPool * pool = (struct LpDictPool *)malloc(sizeof(struct LpDictPool));
	memset(pool, 0, sizeof(struct LpDictPool));
	for (int i = 0; i < DICT_SIZE; i++)
	{
		pool->obj[i].pool = pool;
		if (i == 0)
		{
			pool->obj[i].prev = 0;
			pool->obj[i].next = &pool->obj[i + 1];
		}
		else if (i == DICT_SIZE - 1)
		{
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = 0;
		}
		else {
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = &pool->obj[i + 1];
		}
	}
	pool->head = &pool->obj[0];
	pool->tail = &pool->obj[DICT_SIZE - 1];
	return pool;
}

static struct LpListPool * list_pool_new()
{
	struct LpListPool * pool = (struct LpListPool *)malloc(sizeof(struct LpListPool));
	memset(pool, 0, sizeof(struct LpListPool));
	for (int i = 0; i < LIST_SIZE; i++)
	{
		pool->obj[i].pool = pool;
		if (i == 0)
		{
			pool->obj[i].prev = 0;
			pool->obj[i].next = &pool->obj[i + 1];
		}
		else if (i == LIST_SIZE - 1)
		{
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = 0;
		}
		else {
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = &pool->obj[i + 1];
		}
	}
	pool->head = &pool->obj[0];
	pool->tail = &pool->obj[LIST_SIZE - 1];
	return pool;
}

static struct LpFunPool * func_pool_new()
{
	struct LpFunPool * pool = (struct LpFunPool *)malloc(sizeof(struct LpFunPool));
	memset(pool, 0, sizeof(struct LpFunPool));
	for (int i = 0; i < FUNC_SIZE; i++)
	{
		pool->obj[i].pool = pool;
		if (i == 0)
		{
			pool->obj[i].prev = 0;
			pool->obj[i].next = &pool->obj[i + 1];
		}
		else if (i == FUNC_SIZE - 1)
		{
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = 0;
		}
		else {
			pool->obj[i].prev = &pool->obj[i - 1];
			pool->obj[i].next = &pool->obj[i + 1];
		}
	}
	pool->head = &pool->obj[0];
	pool->tail = &pool->obj[FUNC_SIZE - 1];
	return pool;
}

void init_lp_mem(LP)
{
	lp->obj_pool = obj_pool_new();
	lp->string_pool = 0;
	lp->item_pool = 0;
	lp->obj_array_pool = 0;
	lp->dict_pool = dict_pool_new();
	lp->list_pool = list_pool_new();
	lp->func_pool = func_pool_new();
	lp->lp_None = lp_obj_new(lp, LP_NONE);
	lp->lp_True = lp_number_from_int(lp, 1);
	lp->lp_False = lp_number_from_int(lp, 0);
}

void lp_print_object_pool(LP)
{
	int num[8] = { 0 };

	struct LpObjPool *p = lp->obj_pool;
	while (1)
	{
		lp_obj* n = p->head;
		while (n)
		{
			if (n->ref)
			{
				num[n->type]++;
				if ((num[n->type] % 1000) == 0)
				{
					if (n->type == LP_INT)
						printf("integer : %d\n", n->integer);
					else if(n->type == LP_STRING)
						printf("string : %s\n", n->string.val);
				}
			}
			n = n->next;
		}
		if (!p->next) break;
		p = p->next;
	}

	for (int i = 0; i < 8; i++)
	{
		printf("%d %d\n", i, num[i]);
	}

}

lp_obj* lp_obj_new(LP, int type)
{
	lp_obj* n;
	struct LpObjPool *p = lp->obj_pool;
	while (1)
	{
		if (p->head->ref == 0)
		{
			n = p->head;
			n->type = type;
			n->ref = 1;
			n->prev = p->tail;
			p->tail->next = n;
			p->head = n->next;
			p->head->prev = 0;
			p->tail = n;
			n->next = 0;
			return n;
		}
		if (!p->next) break;
		p = p->next;
	}

	p->next = obj_pool_new();
	p = p->next;
	n = p->head;
	n->type = type;
	n->ref = 1;
	n->prev = p->tail;
	p->tail->next = n;
	p->head = n->next;
	p->head->prev = 0;
	p->tail = n;
	n->next = 0;
	return n;
}

void lp_obj_dec(LP, lp_obj* obj)
{
	int type;
	if (!obj) return;
	type = obj->type;
	if (type < 0)
		return;
	obj->ref--;
	if (obj->ref == 0)
	{
		switch (type)
		{
		case LP_STRING:
			if (obj->string.info)
			{
				obj->string.info->ref--;
				if (obj->string.info->ref == 0)
					lp_string_release(lp, obj->string.info);
			}
			break;
		case LP_LIST:
			obj->type = -obj->type;
			for (int i = 0; i < obj->list->len; i++)
			{
				lp_obj* t = obj->list->items[i];
				lp_obj_dec(lp, t);
			}
			obj->type = -obj->type;
			_lp_list_free(lp, obj->list);
			break;
		case LP_DICT:
			obj->type = -obj->type;
			for (int i = 0; i < obj->dict.val->alloc; i++)
			{
				lp_item *t = &obj->dict.val->items[i];
				if (t->used)
				{
					lp_obj_dec(lp, t->key);
					lp_obj_dec(lp, t->val);
				}
			}
			lp_obj_dec(lp, obj->dict.val->meta);
			obj->type = -obj->type;
			_lp_dict_free(lp, obj->dict.val);
			break;
		case LP_FNC:
			obj->type = -obj->type;
			lp_obj_dec(lp, obj->fnc.info->code);
			lp_obj_dec(lp, obj->fnc.info->self);
			lp_obj_dec(lp, obj->fnc.info->globals);
			lp_fnc_free(lp, obj->fnc.info);
			obj->type = -obj->type;
			break;
		}

		struct LpObjPool *p = (struct LpObjPool *)obj->pool;
		if (obj->prev)
		{
			obj->prev->next = obj->next;
			p->head->prev = obj;
		}
		else {
			return;
		}
		if (obj->next)
		{
			obj->next->prev = obj->prev;
		}
		else {
			p->tail = obj->prev;
		}
		obj->prev = 0;
		obj->next = p->head;
		p->head = obj;
	}
}

static int find_bit_map(int *bit_map, int count, int total)
{
	int i = 0, j = 0, m;
	total -= count;
	while (i <= total)
	{
		m = i + count;
		for (j = i; j < m; j++)
		{
			if (((bit_map[j / 32]) >> (j % 32)) & 1)
				break;
		}
		if (j == m) return i;
		i = j + 1;
	}

	return -1;
}

static void set_bit_map(int *bit_map, int start, int count)
{
	int m = start + count;
	for (int i = start; i < m; i++)
	{
		if (bit_map[i / 32] & (1 << (i % 32)))
		{
			//printf("123\n");
		}
		bit_map[i / 32] |= (1 << (i%32));
	}

	if (count >= 256)
		m = 10;
}

static void reset_bit_map(int *bit_map, int start, int count)
{
	int m = start + count;
	for (int i = start; i < m; i++)
	{
		if (!(bit_map[i / 32] & (1 << (i % 32))))
		{
			//printf("123\n");
		}
		bit_map[i / 32] &= ~(1 << (i % 32));
	}
}

lp_item* lp_item_malloc(LP, int count, void** item_pool, int* item_index)
{
	struct LpItemPool *p;
	int total_size = count;
	if (!lp->item_pool)
	{
		if (total_size < 1024) total_size = 1024;
		else if (total_size < 2048) total_size = 2048;
		else if (total_size < 4096) total_size = 4096;
		lp->item_pool = (struct LpItemPool*)malloc(sizeof(struct LpItemPool) + total_size * sizeof(lp_item));
		lp->item_pool->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
		memset(lp->item_pool->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
		lp->item_pool->next = 0;
		lp->item_pool->total_size = total_size;
		lp->item_pool->not_used = total_size;
	}

	p = lp->item_pool;
	do
	{
		int n = -1;
		if (p->not_used >= count)
			n = find_bit_map(p->bit_map, count, p->total_size);
		if (n >= 0)
		{
			set_bit_map(p->bit_map, n, count);
			p->not_used -= count;
			*item_pool = p;
			*item_index = n;
			memset(&(p->mem[n]), 0, count * sizeof(lp_item));
			return &(p->mem[n]);
		}
		if (!p->next) break;
		p = p->next;
	} while (p);

	if (total_size < 1024) total_size = 1024;
	else if (total_size < 2048) total_size = 2048;
	else if (total_size < 4096) total_size = 4096;
	p->next = (struct LpItemPool*)malloc(sizeof(struct LpItemPool) + total_size * sizeof(lp_item));
	p->next->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
	memset(p->next->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
	set_bit_map(p->next->bit_map, 0, count);
	p->next->next = 0;
	p->next->total_size = total_size;
	p->next->not_used = total_size - count;
	*item_pool = p->next;
	*item_index = 0;
	memset(&(p->next->mem[0]), 0, count * sizeof(lp_item));
	return &(p->next->mem[0]);
}

void lp_item_release(LP, int count, void* item_pool, int item_index)
{
	struct LpItemPool *pool = (struct LpItemPool *)item_pool;
	reset_bit_map(pool->bit_map, item_index, count);
	pool->not_used += count;
}

lp_obj** lp_obj_array_malloc(LP, int count, void** item_pool, int* item_index)
{
	struct LpObjArrayPool *p;
	int total_size = count;
	if (!lp->obj_array_pool)
	{
		if (total_size < 1024) total_size = 1024;
		else if (total_size < 2048) total_size = 2048;
		else if (total_size < 4096) total_size = 4096;
		lp->obj_array_pool = (struct LpObjArrayPool*)malloc(sizeof(struct LpObjArrayPool) + total_size * sizeof(lp_obj*));
		lp->obj_array_pool->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
		memset(lp->obj_array_pool->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
		lp->obj_array_pool->next = 0;
		lp->obj_array_pool->total_size = total_size;
		lp->obj_array_pool->not_used = total_size;
	}

	p = lp->obj_array_pool;
	do
	{
		int n = -1;
		if (p->not_used >= count)
			n = find_bit_map(p->bit_map, count, p->total_size);
		if (n >= 0)
		{
			set_bit_map(p->bit_map, n, count);
			p->not_used -= count;
			*item_pool = p;
			*item_index = n;
			return &(p->mem[n]);
		}
		if (!p->next) break;
		p = p->next;
	} while (p);

	if (total_size < 1024) total_size = 1024;
	else if (total_size < 2048) total_size = 2048;
	else if (total_size < 4096) total_size = 4096;
	p->next = (struct LpObjArrayPool*)malloc(sizeof(struct LpObjArrayPool) + total_size * sizeof(lp_obj*));
	p->next->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
	memset(p->next->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
	set_bit_map(p->next->bit_map, 0, count);
	p->next->next = 0;
	p->next->total_size = total_size;
	p->next->not_used = total_size - count;
	*item_pool = p->next;
	*item_index = 0;
	return &(p->next->mem[0]);
}

void lp_obj_array_release(LP, int count, void* item_pool, int item_index)
{
	struct LpObjArrayPool *pool = (struct LpObjArrayPool *)item_pool;
	reset_bit_map(pool->bit_map, item_index, count);
	pool->not_used += count;
}

_lp_dict* lp_dict_new(LP)
{
	_lp_dict* n;
	struct LpDictPool *p = lp->dict_pool;
	while (1)
	{
		if (p->head->hold == 0)
		{
			n = p->head;
			n->hold = 1;
			n->prev = p->tail;
			p->tail->next = n;
			p->head = n->next;
			p->head->prev = 0;
			p->tail = n;
			n->next = 0;
			return n;
		}
		if (!p->next) break;
		p = p->next;
	}

	p->next = dict_pool_new();
	p = p->next;
	n = p->head;
	n->hold = 1;
	n->prev = p->tail;
	p->tail->next = n;
	p->head = n->next;
	p->head->prev = 0;
	p->tail = n;
	n->next = 0;
	return n;
}

void lp_dict_release(LP, _lp_dict* dict)
{
	struct LpDictPool *p = (struct LpDictPool *)dict->pool;
	dict->hold = 0;
	dict->len = 0;
	dict->alloc = 0;
	dict->meta = 0;
	dict->used = 0;
	if (dict->prev)
	{
		dict->prev->next = dict->next;
		p->head->prev = dict;
	}
	else {
		return;
	}
	if (dict->next)
	{
		dict->next->prev = dict->prev;
	}
	else {
		p->tail = dict->prev;
	}
	dict->prev = 0;
	dict->next = p->head;
	p->head = dict;
}

_lp_list* lp_list_new(LP)
{
	_lp_list* n;
	struct LpListPool *p = lp->list_pool;
	while (1)
	{
		if (p->head->hold == 0)
		{
			n = p->head;
			n->hold = 1;
			n->prev = p->tail;
			p->tail->next = n;
			p->head = n->next;
			p->head->prev = 0;
			p->tail = n;
			n->next = 0;
			return n;
		}
		if (!p->next) break;
		p = p->next;
	}

	p->next = list_pool_new();
	p = p->next;
	n = p->head;
	n->hold = 1;
	n->prev = p->tail;
	p->tail->next = n;
	p->head = n->next;
	p->head->prev = 0;
	p->tail = n;
	n->next = 0;
	return n;
}

void lp_list_release(LP, _lp_list* list)
{
	struct LpListPool *p = (struct LpListPool *)list->pool;
	list->hold = 0;
	if (list->prev)
	{
		list->prev->next = list->next;
		p->head->prev = list;
	}
	else {
		return;
	}
	if (list->next)
	{
		list->next->prev = list->prev;
	}
	else {
		p->tail = list->prev;
	}
	list->prev = 0;
	list->next = p->head;
	p->head = list;
}

_lp_string* lp_string_new(LP, int len)
{
	_lp_string* r;
	struct LpStringPool *p;
	int total_size, size;
	size = len + sizeof(_lp_string);
	total_size = size;
	if (!lp->string_pool)
	{
		if (total_size < 1024) total_size = 1024;
		else if (total_size < 2048) total_size = 2048;
		else if (total_size < 4096) total_size = 4096;
		lp->string_pool = (struct LpStringPool*)malloc(sizeof(struct LpStringPool) + total_size);
		lp->string_pool->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
		memset(lp->string_pool->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
		lp->string_pool->next = 0;
		lp->string_pool->total_size = total_size;
		lp->string_pool->not_used = total_size;
	}

	p = lp->string_pool;
	do
	{
		int n = -1;
		if (p->not_used >= size)
			n = find_bit_map(p->bit_map, size, p->total_size);
		if (n >= 0)
		{
			set_bit_map(p->bit_map, n, size);
			p->not_used -= size;
			r = (_lp_string *)&(p->mem[n]);
			r->ref = 1;
			r->len = len;
			r->pool = p;
			r->index = n;
			return r;
		}
		if (!p->next) break;
		p = p->next;
	} while (p);

	if (total_size < 1024) total_size = 1024;
	else if (total_size < 2048) total_size = 2048;
	else if (total_size < 4096) total_size = 4096;
	p->next = (struct LpStringPool*)malloc(sizeof(struct LpStringPool) + total_size);
	p->next->bit_map = (int*)malloc((total_size / 32 + 1) * sizeof(int));
	memset(p->next->bit_map, 0, (total_size / 32 + 1) * sizeof(int));
	p->next->next = 0;
	p->next->total_size = total_size;
	set_bit_map(p->next->bit_map, 0, size);
	p->next->not_used = total_size - size;
	r = (_lp_string *)&(p->next->mem[0]);
	r->ref = 1;
	r->len = len;
	r->pool = p->next;
	r->index = 0;
	return r;
}

void lp_string_release(LP, _lp_string* string)
{
	struct LpStringPool *pool = (struct LpStringPool *)string->pool;
	int count = string->len + sizeof(_lp_string);
	reset_bit_map(pool->bit_map, string->index, count);
	pool->not_used += count;
}

_lp_fnc* lp_fnc_malloc(LP)
{
	_lp_fnc* n;
	struct LpFunPool *p = lp->func_pool;
	while (1)
	{
		if (p->head->hold == 0)
		{
			n = p->head;
			n->hold = 1;
			n->prev = p->tail;
			p->tail->next = n;
			p->head = n->next;
			p->head->prev = 0;
			p->tail = n;
			n->next = 0;
			return n;
		}
		if (!p->next) break;
		p = p->next;
	}

	p->next = func_pool_new();
	p = p->next;
	n = p->head;
	n->hold = 1;
	n->prev = p->tail;
	p->tail->next = n;
	p->head = n->next;
	p->head->prev = 0;
	p->tail = n;
	n->next = 0;
	return n;
}

void lp_fnc_free(LP, _lp_fnc* fnc)
{
	struct LpFunPool *p = (struct LpFunPool *)fnc->pool;
	fnc->hold = 0;
	if (fnc->prev)
	{
		fnc->prev->next = fnc->next;
		p->head->prev = fnc;
	}
	else {
		return;
	}
	if (fnc->next)
	{
		fnc->next->prev = fnc->prev;
	}
	else {
		p->tail = fnc->prev;
	}
	fnc->prev = 0;
	fnc->next = p->head;
	p->head = fnc;
}