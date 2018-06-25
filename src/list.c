#include "lp.h"
#include "lp_internal.h"

void _lp_list_realloc(LP, _lp_list *self,int len) {
	lp_obj **items = self->items;
	void* item_pool = self->item_pool;
	int item_index = self->item_index;
    if (!len) { len=1; }
    self->items = lp_obj_array_malloc(lp, len, &self->item_pool, &self->item_index);
	if (items)
	{
		memcpy(self->items, items, self->len * sizeof(lp_obj *));
		lp_obj_array_release(lp, self->alloc, item_pool, item_index);
	}
    self->alloc = len;
}

void _lp_list_set(LP,_lp_list *self,int k, lp_obj* v, const char *error) {
    if (k >= self->len) {
        lp_raise(,lp_string(lp, "(_lp_list_set) KeyError"));
    }
    self->items[k] = v;
    LP_OBJ_INC(v);
}
void _lp_list_free(LP, _lp_list *self) {
    lp_obj_array_release(lp, self->alloc, self->item_pool, self->item_index);
    lp_list_release(lp, self);
}

lp_obj* _lp_list_iget(LP, _lp_list *self, int k) {
	if (k >= self->len) {
		RETURN_LP_OBJ(lp->lp_None);
	}
	return self->items[k];
}

lp_obj* _lp_list_get(LP,_lp_list *self,int k,const char *error) {
    if (k >= self->len) {
        lp_raise(0,lp_string(lp, "(_lp_list_get) KeyError"));
    }
	RETURN_LP_OBJ(self->items[k]);
}
void _lp_list_insertx(LP,_lp_list *self, int n, lp_obj* v) {
    if (self->len >= self->alloc) {
        _lp_list_realloc(lp, self,self->alloc*2);
    }
    if (n < self->len) { memmove(&self->items[n+1],&self->items[n],sizeof(lp_obj *)*(self->len-n)); }
    self->items[n] = v;
    self->len += 1;
}
void _lp_list_appendx(LP,_lp_list *self, lp_obj* v) {
    _lp_list_insertx(lp,self,self->len,v);
}
void _lp_list_insert(LP,_lp_list *self, int n, lp_obj* v) {
    _lp_list_insertx(lp,self,n,v);
    LP_OBJ_INC(v);
}
void _lp_list_append(LP,_lp_list *self, lp_obj* v) {
    _lp_list_insert(lp,self,self->len,v);
}
lp_obj* _lp_list_pop(LP,_lp_list *self, int n, const char *error) {
    lp_obj* r = _lp_list_get(lp,self,n,error);
    if (n != self->len-1) { memmove(&self->items[n],&self->items[n+1],sizeof(lp_obj *)*(self->len-(n+1))); }
    self->len -= 1;
	LP_OBJ_DEC(r);
    return r;
}

int _lp_list_find(LP,_lp_list *self, lp_obj* v) {
    int n;
    for (n=0; n<self->len; n++) {
        if (lp_cmp(lp,v,self->items[n]) == 0) {
            return n;
        }
    }
    return -1;
}

lp_obj* lpf_index(LP) {
    lp_obj* self = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
    int i = _lp_list_find(lp,self->list,v);
    if (i < 0) {
        lp_raise(0,lp_string(lp, "(lp_index) ValueError: list.index(x): x not in list"));
    }
    return lp_number_from_int(lp, i);
}

lp_obj* lp_list_copy(LP, lp_obj* rr) {
    lp_obj* val = lp_obj_new(lp, LP_LIST);
    _lp_list *o = rr->list;
    _lp_list *r = lp_list_new(lp);
	r->alloc = o->alloc;
	r->len = o->len;
    r->items = lp_obj_array_malloc(lp, o->alloc, &r->item_pool, &r->item_index);
    memcpy(r->items,o->items,sizeof(lp_obj *)*o->len);
	for (int i = 0; i < o->len; i ++)
	{
		LP_OBJ_INC(r->items[i]);
	}
    val->list = r;
    return val;
}

lp_obj* lpf_append(LP) {
    lp_obj* self = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
    _lp_list_append(lp,self->list,v);
    RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lpf_pop(LP) {
    lp_obj* self = LP_OBJ(0);
    return _lp_list_pop(lp,self->list,self->list->len-1,"pop");
}

lp_obj* lpf_insert(LP) {
    lp_obj* self = LP_OBJ(0);
	int n = LP_INTEGER(1);
    lp_obj* v = LP_OBJ(2);
    _lp_list_insert(lp,self->list,n,v);
	RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lpf_extend(LP) {
    lp_obj* self = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
    int i;
    for (i=0; i<v->list->len; i++) {
        _lp_list_append(lp,self->list,v->list->items[i]);
    }
    RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lp_list_nt(LP) {
    lp_obj* r = lp_obj_new(lp, LP_LIST);
	r->list = lp_list_new(lp);
    r->list->items = 0;
	r->list->len = 0;
	r->list->alloc = 0;
    return r;
}

lp_obj* lp_list(LP) {
    lp_obj* r = lp_obj_new(lp, LP_LIST);
	r->list = lp_list_new(lp);
	r->list->items = 0;
	r->list->len = 0;
	r->list->alloc = 0;
    return r;
}

lp_obj* lp_list_n(LP,int n,lp_obj **argv) {
    int i;
    lp_obj* r = lp_list(lp); _lp_list_realloc(lp, r->list,n);
    for (i=0; i<n; i++) {
        _lp_list_append(lp,r->list,argv[i]);
    }
    return r;
}

int _lp_sort_cmp(lp_obj **a,lp_obj **b) {
    return lp_cmp(0,*a,*b);
}

lp_obj* lpf_sort(LP) {
    lp_obj* self = LP_OBJ(0);
    qsort(self->list->items, self->list->len, sizeof(lp_obj *), (int(*)(const void**,const void**))_lp_sort_cmp);
    RETURN_LP_OBJ(lp->lp_None);
}

