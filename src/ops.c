#include "lp.h"
#include "lp_internal.h"

/* File: Operations
 * Various tinypy operations.
 */

/* Function: lp_str
 * String representation of an object.
 *
 * Returns a string object representating self.
 */
lp_obj* lp_str(LP,lp_obj* self) {
    int type = self->type;
    if (type == LP_STRING) { RETURN_LP_OBJ(self); }
	else if (type == LP_INT) {
		return lp_printf(lp, "%d", self->integer);
	} else if (type == LP_DOUBLE) {
        return lp_printf(lp,"%f", self->doublen);
    } else if(type == LP_DICT) {
        return lp_printf(lp,"<dict 0x%x>",self->dict);
    } else if(type == LP_LIST) {
        return lp_printf(lp,"<list 0x%x>",self->list);
    } else if (type == LP_NONE) {
        return lp_string(lp, "None");
    } else if (type == LP_DATA) {
        return lp_printf(lp,"<data 0x%x>",self->data.val);
    } else if (type == LP_FNC) {
        return lp_printf(lp,"<fnc 0x%x>",self->fnc.info);
    }
    return lp_string(lp, "<?>");
}

/* Function: lp_bool
 * Check the truth value of an object
 *
 * Returns false if v is a numeric object with a value of exactly 0, v is of
 * type None or v is a string list or dictionary with a length of 0. Else true
 * is returned.
 */
int lp_bool(LP,lp_obj* v) {
    switch(v->type) {
        case LP_INT: return v->integer != 0;
		case LP_DOUBLE:	return v->doublen != 0.0;
        case LP_NONE: return 0;
        case LP_STRING: return v->string.len != 0;
        case LP_LIST: return v->list->len != 0;
        case LP_DICT: return v->dict.val->len != 0;
    }
    return 1;
}


/* Function: lp_has
 * Checks if an object contains a key.
 *
 * Returns lp_True if self[k] exists, lp_False otherwise.
 */
lp_obj* lp_has(LP,lp_obj* self, lp_obj* k) {
    int type = self->type;
    if (type == LP_DICT) {
        if (_lp_dict_find(lp,self->dict.val,k) != -1) { RETURN_LP_OBJ(lp->lp_True); }
		RETURN_LP_OBJ(lp->lp_False);
    } else if (type == LP_STRING && k->type == LP_STRING) {
        return lp_number_from_int(lp, _lp_str_index(self,0,k)!=-1);
    } else if (type == LP_LIST) {
        return lp_number_from_int(lp, _lp_list_find(lp,self->list,k)!=-1);
    }
    lp_raise(0,lp_string(lp, "(lp_has) TypeError: iterable argument required"));
}

/* Function: lp_del
 * Remove a dictionary entry.
 *
 * Removes the key k from self. Also works on classes and objects.
 *
 * Note that unlike with Python, you cannot use this to remove list items.
 */
void lp_del(LP,lp_obj* self, lp_obj* k) {
    int type = self->type;
    if (type == LP_DICT) {
        _lp_dict_del(lp,self->dict.val,k,"lp_del");
        return;
    }
    lp_raise(,lp_string(lp, "(lp_del) TypeError: object does not support item deletion"));
}


/* Function: lp_iter
 * Iterate through a list or dict.
 *
 * If self is a list/string/dictionary, this will iterate over the
 * elements/characters/keys respectively, if k is an increasing index
 * starting with 0 up to the length of the object-1.
 *
 * In the case of a list of string, the returned items will correspond to the
 * item at index k. For a dictionary, no guarantees are made about the order.
 * You also cannot call the function with a specific k to get a specific
 * item -- it is only meant for iterating through all items, calling this
 * function len(self) times. Use <lp_get> to retrieve a specific item, and
 * <lp_len> to get the length.
 *
 * Parameters:
 * self - The object over which to iterate.
 * k - You must pass 0 on the first call, then increase it by 1 after each call,
 *     and don't call the function with k >= len(self).
 *
 * Returns:
 * The first (k = 0) or next (k = 1 .. len(self)-1) item in the iteration.
 */
lp_obj* lp_iter(LP,lp_obj* self, lp_obj* k) {
    int type = self->type;
    if (type == LP_LIST || type == LP_STRING) { return lp_get(lp,self,k); }
    if (type == LP_DICT && k->type == LP_INT) {
        lp_obj* obj = self->dict.val->items[_lp_dict_next(lp,self->dict.val)].key;
		RETURN_LP_OBJ(obj);
    }
    lp_raise(0,lp_string(lp, "(lp_iter) TypeError: iteration over non-sequence"));
}


/* Function: lp_get
 * Attribute lookup.
 * 
 * This returns the result of using self[k] in actual code. It works for
 * dictionaries (including classes and instantiated objects), lists and strings.
 *
 * As a special case, if self is a list, self[None] will return the first
 * element in the list and subsequently remove it from the list.
 */
lp_obj* lp_get(LP,lp_obj* self, lp_obj* k) {
    int type = self->type;
    lp_obj* r;
    if (type == LP_DICT) {
        LP_META_BEGIN(self,"__get__");
			lp_params_v(lp, 1, k);
            r = lp_call(lp,meta);
			LP_OBJ_DEC(meta);
			return r;
        LP_META_END;
        if (self->dict.dtype && lp_lookup(lp,self,k,&r)) { return r; }
        return _lp_dict_get(lp,self->dict.val,k,"lp_get");
    } else if (type == LP_LIST) {
        if (k->type == LP_INT) {
            int l = lp_lenx(lp,self);
            int n = k->integer;
            n = (n<0?l+n:n);
            return _lp_list_get(lp,self->list,n,"lp_get");
        } else if (k->type == LP_STRING) {
            if (_lp_str_cmp(k, "append") == 0) {
                return lp_method(lp,self,lpf_append);
            } else if (_lp_str_cmp(k, "pop") == 0) {
                return lp_method(lp,self,lpf_pop);
            } else if (_lp_str_cmp(k, "index") == 0) {
                return lp_method(lp,self,lpf_index);
            } else if (_lp_str_cmp(k, "sort") == 0) {
                return lp_method(lp,self,lpf_sort);
            } else if (_lp_str_cmp(k, "extend") == 0) {
                return lp_method(lp,self,lpf_extend);
            } else if (_lp_str_cmp(k, "*") == 0) {
                lp_params_v(lp,1,self);
                r = lpf_copy(lp);
                self->list->len=0;
                return r;
            }
        } else if (k->type == LP_NONE) {
            return _lp_list_pop(lp,self->list,0,"lp_get");
        }
    } else if (type == LP_STRING) {
        if (k->type == LP_INT) {
            int l = self->string.len;
            int n = k->integer;
            n = (n<0?l+n:n);
            if (n >= 0 && n < l) { return lp_string_n(lp, lp->chars[(unsigned char)self->string.val[n]],1); }
        } else if (k->type == LP_STRING) {
            if (lp_cmps(lp, k, "join") == 0) {
                return lp_method(lp,self,lpf_join);
            } else if (lp_cmps(lp, k, "split") == 0) {
                return lp_method(lp,self,lpf_split);
            } else if (lp_cmps(lp, k, "index") == 0) {
                return lp_method(lp,self,lpf_str_index);
            } else if (lp_cmps(lp, k, "strip") == 0) {
                return lp_method(lp,self,lpf_strip);
            } else if (lp_cmps(lp, k, "replace") == 0) {
                return lp_method(lp,self,lpf_replace);
            }
        }
    }

    if (k->type == LP_LIST) {
        int a,b,l;
        lp_obj* tmp;
		l = lp_lenx(lp, self);
        tmp = _lp_list_get(lp, k->list, 0, "lp_get");
		if (tmp->type == LP_INT) { a = tmp->integer; }
        else if(tmp->type == LP_NONE) { a = 0; }
        else { LP_OBJ_DEC(tmp); lp_raise(0,lp_string(lp, "(lp_get) TypeError: indices must be integer")); }
		LP_OBJ_DEC(tmp);
        tmp = _lp_list_get(lp, k->list, 1, "lp_get");
        if (tmp->type == LP_INT) { b = tmp->integer; }
        else if(tmp->type == LP_NONE) { b = l; }
        else { LP_OBJ_DEC(tmp); lp_raise(0,lp_string(lp, "(lp_get) TypeError: indices must be integer")); }
		LP_OBJ_DEC(tmp);
        a = _lp_max(0,(a<0?l+a:a)); b = _lp_min(l,(b<0?l+b:b));
        if (type == LP_LIST) {
            return lp_list_n(lp,b-a,&self->list->items[a]);
        } else if (type == LP_STRING) {
            return lp_string_sub(lp,self,a,b);
        }
    }

    lp_raise(0,lp_string(lp, "(lp_get) TypeError: ?"));
}

lp_obj* lp_getk(LP, lp_obj* self, lp_obj* k)
{
	lp_obj* r = lp_get(lp, self, k);
	LP_OBJ_DEC(k);
	return r;
}

/* Function: lp_iget
 * Failsafe attribute lookup.
 *
 * This is like <lp_get>, except it will return false if the attribute lookup
 * failed. Otherwise, it will return true, and the object will be returned
 * over the reference parameter r.
 */
int lp_iget(LP,lp_obj **r, lp_obj* self, lp_obj* k) {
    if (self->type == LP_DICT) {
        int n = _lp_dict_find(lp,self->dict.val,k);
        if (n == -1) { return 0; }
        *r = self->dict.val->items[n].val;
		LP_OBJ_INC(*r);
        return 1;
    }
    if (self->type == LP_LIST && !self->list->len) { return 0; }
    *r = lp_get(lp,self,k);
    return 1;
}

int lp_igetk(LP, lp_obj **r, lp_obj* self, lp_obj* k)
{
	int result = lp_iget(lp, r, self, k);
	LP_OBJ_DEC(k);
	return result;
}

/* Function: lp_set
 * Attribute modification.
 * 
 * This is the counterpart of lp_get, it does the same as self[k] = v would do
 * in actual tinypy code.
 */
void lp_set(LP,lp_obj* self, lp_obj* k, lp_obj* v) {
    int type = self->type;
	lp_obj* r;

    if (type == LP_DICT) {
        LP_META_BEGIN(self, "__set__");
			lp_params_v(lp, 2, k, v);
            r = lp_call(lp,meta);
			LP_OBJ_DEC(meta);
			LP_OBJ_DEC(r);
            return;
        LP_META_END;
        _lp_dict_set(lp,self->dict.val,k,v);
        return;
    } else if (type == LP_LIST) {
        if (k->type == LP_INT) {
            _lp_list_set(lp,self->list, k->integer,v,"lp_set");
            return;
        } else if (k->type == LP_NONE) {
            _lp_list_append(lp,self->list,v);
            return;
        } else if (k->type == LP_STRING) {
			lp_obj* name = lp_string(lp, "*");
            if (lp_cmp(lp,name,k) == 0) {
                lp_params_v(lp,2,self,v);lpf_extend(lp);
				LP_OBJ_DEC(name);
                return;
            }
			LP_OBJ_DEC(name);
        }
    }
    lp_raise(,lp_string(lp, "(lp_set) TypeError: object does not support item assignment"));
}

void lp_setk(LP, lp_obj* self, lp_obj* k, lp_obj* v)
{
	lp_set(lp, self, k, v);
	LP_OBJ_DEC(k);
}

void lp_setv(LP, lp_obj* self, lp_obj* k, lp_obj* v)
{
	lp_set(lp, self, k, v);
	LP_OBJ_DEC(v);
}

void lp_setkv(LP, lp_obj* self, lp_obj* k, lp_obj* v)
{
	lp_set(lp, self, k, v);
	LP_OBJ_DEC(k);
	LP_OBJ_DEC(v);
}

lp_obj* lp_add(LP,lp_obj* a, lp_obj* b) {
    if (lp_is_number(a) && lp_is_number(b)) {
		if (a->type == LP_INT && a->type == b->type)
			return lp_number_from_int(lp, a->integer + b->integer);
		else
			return lp_number_from_double(lp, lp_type_number(lp, a) + lp_type_number(lp, b));
    } else if (a->type == LP_STRING && a->type == b->type) {
        int al = a->string.len, bl = b->string.len;
        lp_obj* r = lp_string_t(lp,al+bl);
        char *s = r->string.info->s;
        memcpy(s,a->string.val,al); memcpy(s+al,b->string.val,bl);
        return r;
    } else if (a->type == LP_LIST && a->type == b->type) {
        lp_obj* r;
        lp_params_v(lp,1,a);
        r = lpf_copy(lp);
        lp_params_v(lp,2,r,b);
        lpf_extend(lp);
        return r;
    }
    lp_raise(0,lp_string(lp, "(lp_add) TypeError: ?"));
}

lp_obj* lp_mul(LP,lp_obj* a, lp_obj* b) {
    if (lp_is_number(a) && lp_is_number(b)) {
		if (a->type == LP_INT && a->type == b->type)
			return lp_number_from_int(lp, a->integer * b->integer);
		else
			return lp_number_from_double(lp, lp_type_number(lp, a) * lp_type_number(lp, b));
    } else if ((a->type == LP_STRING && (b->type == LP_INT)) ||
               ((a->type == LP_INT) && b->type == LP_STRING)) {
        if(a->type == LP_INT) {
            lp_obj* c = a; a = b; b = c;
        }
        int al = a->string.len; int n = b->integer;
        if(n <= 0) {
            lp_obj* r = lp_string_t(lp,0);
            return r;
        }
        lp_obj* r = lp_string_t(lp,al*n);
        char *s = r->string.info->s;
        int i; for (i=0; i<n; i++) { memcpy(s+al*i,a->string.val,al); }
        return r;
    }
    lp_raise(0,lp_string(lp, "(lp_mul) TypeError: ?"));
}

/* Function: lp_len
 * Returns the length of an object.
 *
 * Returns the number of items in a list or dict, or the length of a string.
 */
lp_obj* lp_len(LP,lp_obj* self) {
    int type = self->type;
    if (type == LP_STRING) {
        return lp_number_from_int(lp, self->string.len);
    } else if (type == LP_DICT) {
        return lp_number_from_int(lp, self->dict.val->len);
    } else if (type == LP_LIST) {
        return lp_number_from_int(lp, self->list->len);
    }
    
    lp_raise(0,lp_string(lp, "(lp_len) TypeError: len() of unsized object"));
}

int lp_lenx(LP, lp_obj* self)
{
	int type = self->type;
	if (type == LP_STRING) {
		return self->string.len;
	}
	else if (type == LP_DICT) {
		return self->dict.val->len;
	}
	else if (type == LP_LIST) {
		return self->list->len;
	}

	lp_raise(0, lp_string(lp, "(lp_len) TypeError: len() of unsized object"));
}

int lp_cmp(LP,lp_obj* a, lp_obj* b) {
    if (a->type != b->type) { return a->type-b->type; }
    switch(a->type) {
        case LP_NONE: return 0;
		case LP_INT:
        case LP_DOUBLE: return _lp_sign(lp_type_number(lp, a)- lp_type_number(lp, b));
        case LP_STRING: {
            int l = _lp_min(a->string.len,b->string.len);
            int v = memcmp(a->string.val,b->string.val,l);
            if (v == 0) {
                v = a->string.len-b->string.len;
            }
            return v;
        }
        case LP_LIST: {
            int n,v; for(n=0;n<_lp_min(a->list->len,b->list->len);n++) {
        lp_obj* aa = a->list->items[n]; lp_obj* bb = b->list->items[n];
            if (aa->type == LP_LIST && bb->type == LP_LIST) { v = aa->list-bb->list; } else { v = lp_cmp(lp,aa,bb); }
            if (v) { return v; } }
            return a->list->len-b->list->len;
        }
        case LP_DICT: return a->dict.val - b->dict.val;
        case LP_FNC: return a->fnc.info - b->fnc.info;
        case LP_DATA: return (char*)a->data.val - (char*)b->data.val;
    }
    lp_raise(0,lp_string(lp, "(lp_cmp) TypeError: ?"));
}

int lp_cmps(LP, lp_obj* a, const char* b)
{
	if (a->type != LP_STRING) { return a->type - LP_STRING; }

	int l = _lp_min(a->string.len, strlen(b));
	int v = memcmp(a->string.val, b, l);
	if (v == 0) {
		v = a->string.len - strlen(b);
	}
	return v;
}

#define LP_INT_OP(name,expr) \
    lp_obj* name(LP,lp_obj* _a,lp_obj* _b) { \
    if (_a->type == LP_INT && _a->type == _b->type) { \
        int a = _a->integer; int b = _b->integer; \
        return lp_number_from_int(lp, expr); \
    } \
    lp_raise(0,lp_string(lp, "(" #name ") TypeError: unsupported operand type(s)")); \
}

#define LP_OP(name,expr) \
    lp_obj* name(LP,lp_obj* _a,lp_obj* _b) { \
    if (lp_is_number(_a) && lp_is_number(_b)) { \
		if (_a->type == LP_INT && _a->type == _b->type) \
		{ \
			int a = _a->integer; int b = _b->integer; \
			return lp_number_from_int(lp, expr); \
		} \
		else \
		{ \
			double a = lp_type_number(lp, _a); double b = lp_type_number(lp, _b); \
			return lp_number_from_double(lp, expr); \
		} \
    } \
    lp_raise(0,lp_string(lp, "(" #name ") TypeError: unsupported operand type(s)")); \
}

LP_INT_OP(lp_bitwise_and,(a)&(b));
LP_INT_OP(lp_bitwise_or,(a)|(b));
LP_INT_OP(lp_bitwise_xor,(a)^(b));
LP_INT_OP(lp_lsh,(a)<<(b));
LP_INT_OP(lp_rsh,(a)>>(b));
LP_INT_OP(lp_mod,(a)%(b));
LP_OP(lp_sub,a-b);
LP_OP(lp_div,a/b);
LP_OP(lp_pow,pow(a,b));

lp_obj* lp_bitwise_not(LP, lp_obj* a) {
    if (a->type == LP_INT) {
        return lp_number_from_int(lp, ~a->integer);
    }
    lp_raise(0,lp_string(lp, "(lp_bitwise_not) TypeError: unsupported operand type"));
}

/**/
