#include "lp.h"
#include "lp_internal.h"

/* File: Dict
 * Functions for dealing with dictionaries.
 */
int _lua_hash(void const *v,int l) {
    int i,step = (l>>5)+1;
    int h = l + (l >= 4?*(int*)v:0);
    for (i=l; i>=step; i-=step) {
        h = h^((h<<5)+(h>>2)+((unsigned char *)v)[i-1]);
    }
    return h;
}
void _lp_dict_free(LP, lp_dict *self) {
    lp_item_release(lp, self->alloc, self->item_pool, self->item_index);
    lp_dict_release(lp, self);
}

/* void _lp_dict_reset(_lp_dict *self) {
       memset(self->items,0,self->alloc*sizeof(lp_item));
       self->len = 0;
       self->used = 0;
       self->cur = 0;
   }*/

int lp_hash(LP,lp_obj v) {
    switch (v & 0x7) {
        case LP_NONE: return 0;
		case LP_INT: {
			double d = lp_obj_to_int(lp, v);
			return _lua_hash(&d, sizeof(double));
		}
		case LP_DOUBLE: {
			double d = lp_obj_to_double(lp, v);
			return _lua_hash(&d, sizeof(double));
		}
		case LP_STRING: {
			lp_string* string = lp_obj_to_string(lp, v);
			return _lua_hash(string->val, string->len);
		}
    }
    lp_raise(0,lp_string_new(lp, "(lp_hash) TypeError: value unhashable"));
}

void _lp_dict_hash_set(LP,lp_dict *self, int hash, lp_obj k, lp_obj v) {
    lp_item item;
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used > 0) { continue; }
        if (self->items[n].used == 0) { self->used += 1; }
        item.used = 1;
        item.hash = hash;
        item.key = k;
		LP_OBJ_INC(k);
        item.val = v;
		LP_OBJ_INC(v);
        self->items[n] = item;
        self->len += 1;
        return;
    }
    lp_raise(,lp_string_new(lp, "(_lp_dict_hash_set) RuntimeError: ?"));
}

void _lp_dict_lp_realloc(LP,lp_dict *self,int len) {
    lp_item *items = self->items;
	void* item_pool = self->item_pool;
	int item_index = self->item_index;
    int i,alloc = self->alloc;
    len = _lp_max(8,len);

    self->items = lp_item_malloc(lp, len, &self->item_pool, &self->item_index);
    self->alloc = len; self->mask = len-1;
    self->len = 0; self->used = 0;

    for (i=0; i<alloc; i++) {
        if (items[i].used != 1) { continue; }
        _lp_dict_hash_set(lp,self,items[i].hash,items[i].key,items[i].val);
    }
	
	if (items)
	{
		lp_item_release(lp, alloc, item_pool, item_index);
	}
}

int _lp_dict_hash_find(LP,lp_dict *self, int hash, lp_obj k) {
    int i,idx = hash&self->mask;
    for (i=idx; i<idx+self->alloc; i++) {
        int n = i&self->mask;
        if (self->items[n].used == 0) { break; }
        if (self->items[n].used < 0) { continue; }
        if (self->items[n].hash != hash) { continue; }
        if (lp_cmp(lp,self->items[n].key,k) != 0) { continue; }
        return n;
    }
    return -1;
}
int _lp_dict_find(LP,lp_dict *self,lp_obj k) {
    return _lp_dict_hash_find(lp,self,lp_hash(lp,k),k);
}

void _lp_dict_set(LP,lp_dict *self,lp_obj k, lp_obj v) {
    int hash = lp_hash(lp,k); int n = _lp_dict_hash_find(lp,self,hash,k);
    if (n == -1) {
        if (self->len >= (self->alloc/2)) {
            _lp_dict_lp_realloc(lp,self,self->alloc*2);
        } else if (self->used >= (self->alloc*3/4)) {
            _lp_dict_lp_realloc(lp,self,self->alloc);
        }
        _lp_dict_hash_set(lp,self,hash,k,v);
    } else {
		LP_OBJ_DEC(self->items[n].val);
        self->items[n].val = v;
		LP_OBJ_INC(v);
    }
}

void _lp_dict_setx(LP,lp_dict *self,lp_obj k, lp_obj v) {
    _lp_dict_setx(lp,self,k,v);
   LP_OBJ_DEC(k);
   LP_OBJ_DEC(v);
}

lp_obj _lp_dict_get(LP,lp_dict *self,lp_obj k, const char *error) {
    int n = _lp_dict_find(lp,self,k);
    if (n < 0) {
        lp_raise(0,lp_add(lp,lp_string_new(lp, "(_lp_dict_get) KeyError: "),lp_str(lp,k)));
    }
    RETURN_LP_OBJ(self->items[n].val);
}

void _lp_dict_del(LP,lp_dict *self,lp_obj k, const char *error) {
    int n = _lp_dict_find(lp,self,k);
    if (n < 0) {
        lp_raise(,lp_add(lp,lp_string_new(lp, "(_lp_dict_del) KeyError: "),lp_str(lp,k)));
    }
    self->items[n].used = -1;
	if (!self->items[n].used)
	{
		LP_OBJ_DEC(self->items[n].key);
		LP_OBJ_DEC(self->items[n].val);
	}
    self->len -= 1;
}

lp_obj lp_dict_copy(LP,lp_obj rr) {
    lp_obj obj = LP_DICT;
    lp_dict *o = tp_obj_to_dict(rr);
    lp_dict *r = _lp_dict_new(lp);
	r->alloc = o->alloc;
	r->cur = o->cur;
	r->len = o->len;
	r->mask = o->mask;
	r->meta = o->meta;
	LP_OBJ_INC(o->meta);
    r->items = lp_item_malloc(lp, o->alloc, &r->item_pool, &r->item_index);
    memcpy(r->items,o->items,sizeof(lp_item)*o->alloc);
	for (int i = 0; i < o->len; i++)
	{
		LP_OBJ_INC(r->items[i].key);
		LP_OBJ_INC(r->items[i].val);
	}
    r->dtype = 1;
    RETURN_LP_OBJ_NEW(r, LP_DICT);
}

int _lp_dict_next(LP,lp_dict *self) {
    if (!self->len) {
        lp_raise(0,lp_string_new(lp, "(_lp_dict_next) RuntimeError"));
    }
    while (1) {
        self->cur = ((self->cur + 1) & self->mask);
        if (self->items[self->cur].used > 0) {
            return self->cur;
        }
    }
}

lp_obj lpf_merge(LP) {
    lp_obj* self = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
	lp_dict* self_d = lp_obj_to_dict(self);
	lp_dict* dict = lp_obj_to_dict(v);
    int i; for (i=0; i<dict->len; i++) {
        int n = _lp_dict_next(lp,dict);
        _lp_dict_set(lp,self_d,
            dict->items[n].key,dict->items[n].val);
    }
    RETURN_LP_NONE;
}

/* Function: lp_dict
 *
 * Creates a new dictionary object.
 *
 * *Note* If you use <lp_setmeta> on the dictionary, you have to use <lp_getraw> to
 * access the "raw" dictionary again.
 *
 * Returns:
 * The newly created dictionary.
 */
lp_obj lp_dict_new(LP) {
    lp_dict* dict = _lp_dict_new(lp);
	dict->items = 0;
	dict->len = 0;
	dict->alloc = 0;
	dict->mask = 0;
	dict->meta = 0;
    dict->dtype = 1;
    RETURN_LP_OBJ_NEW(dict, LP_DICT);
}

lp_obj lp_dict_n(LP,int n, lp_obj** argv) {
    lp_obj r = lp_dict_new(lp);
    int i; for (i=0; i<n; i++) { lp_set(lp,r,argv[i*2],argv[i*2+1]); }
    return r;
}


