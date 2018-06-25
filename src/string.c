#include "lp.h"
#include "lp_internal.h"

/* File: String
 * String handling functions.
 */
 
/*
 * Create a new string which is a copy of some memory.
 * This is put into GC tracking for you.
 */
lp_obj lp_string_copy(LP, const char *s, int n) {
    lp_string* string = _lp_string_new(lp,n);
    memcpy(string->s,s,n);
    RETUNRN_LP_OBJ_NEW(string, LP_STRING);
}

/*
 * Create a new string which is a substring slice of another STRING.
 * Does not need to be put into GC tracking, as its parent is
 * already being tracked (supposedly).
 */
lp_obj lp_string_sub(LP, lp_obj s, int a, int b) {
	lp_string* string = lp_obj_to_string(s);
    int l = string->len;
    a = _lp_max(0,(a<0?l+a:a)); b = _lp_min(l,(b<0?l+b:b));
	lp_string* r = _lp_string_new(lp, 0);
	r->parent = string;
	string->ref++;
    r->val = string->val + a;
    r->len = b-a;
    RETUNRN_LP_OBJ_NEW(r, LP_STRING);
}

lp_obj lp_printf(LP, char const *fmt,...) {
    int l;
    lp_string* r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = _lp_string_new(lp,l);
    s = r->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
	RETUNRN_LP_OBJ_NEW(r, LP_STRING);
}


int _lp_str_cmp(lp_obj s, const char* k) {
	int l = strlen(k);
	lp_string* string = lp_obj_to_string(s);
	int n = _lp_min(string->len, l);
	int v = memcmp(string->val, k, n);
	if (v == 0) v = string->len - l;
	return v;
}

int _lp_str_index(lp_obj s, int i, lp_obj k) {
	lp_string* s_str = lp_obj_to_string(s);
	lp_string* k_str = lp_obj_to_string(k);
    while ((s_str->len - i) >= k_str->len) {
        if (memcmp(s_str->val+i, k_str->val, k_str->len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}


lp_obj lpf_join(LP) {
    lp_obj delim = LP_OBJ(0);
    lp_obj val = LP_OBJ(1);
	lp_string* string = lp_obj_to_string(delim);
	lp_list* list = tp_obj_to_list(val);
    int l=0,i;
    lp_string* r;
    char *s;
    for (i=0; i<list->len; i++) {
        if (i!=0) { l += string->len; }
		r = lp_str(lp, list->items[i]);
        l += string->len;
		lp_obj_dec(lp, r);
    }
    r = _lp_string_new(lp,l);
    s = string->s;
    l = 0;
    for (i=0; i<list->len; i++) {
        lp_obj e;
        if (i!=0) {
            memcpy(s+l,string->val,string->len); l += string->len;
        }
        e = list->items[i];
		if ((e & 7) == LP_STRING)
		{
			lp_string* elem = lp_obj_to_string(e);
			memcpy(s + l, elem->val, elem->len); l += elem->len;
		}
    }
	RETUNRN_LP_OBJ_NEW(r, LP_STRING);
}

lp_obj lpf_split(LP) {
    lp_obj v = LP_OBJ(0);
    lp_obj d = LP_OBJ(1);
	lp_string *string = lp_obj_to_string(d);
    lp_list* list = _lp_list_new(lp);

    int i = 0;
    while ((i=_lp_str_index(v,i,d))!=-1) {
        _lp_list_append(lp,list,lp_string_sub(lp,v,0,i));
        i += string->len;
    }
    RETURN_LP_OBJ_NEW(list, LP_LIST);
}


lp_obj lpf_find(LP) {
    lp_obj s = LP_OBJ(0);
    lp_obj v = LP_OBJ(1);
    return lp_number_from_int(lp, _lp_str_index(s,0,v));
}

lp_obj lpf_str_index(LP) {
    lp_obj s = LP_OBJ(0);
    lp_obj v = LP_OBJ(1);
    int n = _lp_str_index(s,0,v);
    if (n >= 0) { return lp_number_from_int(lp, n); }
    lp_raise(0,lp_string_new(lp, "(lp_str_index) ValueError: substring not found"));
}

lp_obj lpf_str2(LP) {
    lp_obj v = LP_OBJ(0);
    return lp_str(lp,v);
}

lp_obj lpf_chr(LP) {
	int v = LP_INTEGER(0);
    return lp_string_n(lp, lp->chars[(unsigned char)v],1);
}
lp_obj lpf_ord(LP) {
    lp_obj s = LP_STR(0);
	lp_string* string = lp_obj_to_string(s);
    if (string->len != 1) {
        lp_raise(0,lp_string_new(lp, "(lp_ord) TypeError: ord() expected a character"));
    }
    return lp_number_from_int(lp, (unsigned char)string->val[0]);
}

lp_obj lpf_strip(LP) {
    lp_obj o = LP_TYPE(0, LP_STRING);
	lp_string* string = lp_obj_to_string(o);
    char const *v = string->val; int l = string->len;
    int i; int a = l, b = 0;
	lp_string* r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _lp_min(a,i); b = _lp_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return lp_string_new(lp, ""); }
    r = lp_string_t(lp,b-a);
    s = r->s;
    memcpy(s,v+a,b-a);
	RETURN_LP_OBJ_NEW(r, LP_STRING);
}

lp_obj lpf_replace(LP) {
    lp_obj s = LP_OBJ(0);
    lp_obj k = LP_OBJ(1);
    lp_obj v = LP_OBJ(2);
    int i = 0,n = 0;
    int c;
    int l;
    lp_obj rr;
    char *r;
    char *d;
    while ((i = _lp_str_index(s,i,k)) != -1) {
        n += 1;
        i += k->string.len;
    }
/*     fprintf(stderr,"ns: %d\n",n); */
    l = s->string.len + n * (v->string.len-k->string.len);
    rr = lp_string_t(lp,l);
    r = rr->string.info->s;
    d = r;
	i = 0;  //
    while ((n = _lp_str_index(s,i,k)) != -1) {
        memcpy(d,s->string.val+i,c=(n-i)); d += c;
        i = n + k->string.len;
        memcpy(d,v->string.val,v->string.len); d += v->string.len;
    }
    memcpy(d,s->string.val+i,s->string.len-i);

    return rr;
}

