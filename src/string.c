#include "lp.h"
#include "lp_internal.h"

/* File: String
 * String handling functions.
 */
 
/*
 * Create a new empty string of a certain size.
 * Does not put it in for GC tracking, since contents should be
 * filled after returning.
 */ 
lp_obj* lp_string_t(LP, int n) {
    lp_obj* r = lp_string_n(lp, 0,n);
    r->string.info = lp_string_new(lp, n);
	r->string.info->ref = 1;
    r->string.val = r->string.info->s;
	r->string.len = n;
    return r;
}

/*
 * Create a new string which is a copy of some memory.
 * This is put into GC tracking for you.
 */
lp_obj* lp_string_copy(LP, const char *s, int n) {
    lp_obj* r = lp_string_t(lp,n);
    memcpy(r->string.info->s,s,n);
    return r;
}

/*
 * Create a new string which is a substring slice of another STRING.
 * Does not need to be put into GC tracking, as its parent is
 * already being tracked (supposedly).
 */
lp_obj* lp_string_sub(LP, lp_obj* s, int a, int b) {
    int l = s->string.len;
    a = _lp_max(0,(a<0?l+a:a)); b = _lp_min(l,(b<0?l+b:b));
    lp_obj* r = lp_obj_new(lp, LP_STRING);
	r->string.info = s->string.info;
	if (r->string.info)
		r->string.info->ref++;
    r->string.val = s->string.val + a;
    r->string.len = b-a;
    return r;
}

lp_obj* lp_printf(LP, char const *fmt,...) {
    int l;
    lp_obj* r;
    char *s;
    va_list arg;
    va_start(arg, fmt);
    l = vsnprintf(NULL, 0, fmt,arg);
    r = lp_string_t(lp,l);
    s = r->string.info->s;
    va_end(arg);
    va_start(arg, fmt);
    vsprintf(s,fmt,arg);
    va_end(arg);
    return r;
}


int _lp_str_cmp(lp_obj* s, const char* k) {
	int l = strlen(k);
	int n = _lp_min(s->string.len, l);
	int v = memcmp(s->string.val, k, n);
	if (v == 0) v = s->string.len - l;
	return v;
}

int _lp_str_index(lp_obj* s, int i, lp_obj* k) {
    while ((s->string.len - i) >= k->string.len) {
        if (memcmp(s->string.val+i,k->string.val,k->string.len) == 0) {
            return i;
        }
        i += 1;
    }
    return -1;
}


lp_obj* lpf_join(LP) {
    lp_obj* delim = LP_OBJ(0);
    lp_obj* val = LP_OBJ(1);
    int l=0,i;
    lp_obj* r;
    char *s;
    for (i=0; i<val->list->len; i++) {
        if (i!=0) { l += delim->string.len; }
		r = lp_str(lp, val->list->items[i]);
        l += r->string.len;
		lp_obj_dec(lp, r);
    }
    r = lp_string_t(lp,l);
    s = r->string.info->s;
    l = 0;
    for (i=0; i<val->list->len; i++) {
        lp_obj* e;
        if (i!=0) {
            memcpy(s+l,delim->string.val,delim->string.len); l += delim->string.len;
        }
        e = lp_str(lp,val->list->items[i]);
        memcpy(s+l,e->string.val,e->string.len); l += e->string.len;
		lp_obj_dec(lp, e);
    }
    return r;
}

lp_obj* lpf_split(LP) {
    lp_obj* v = LP_OBJ(0);
    lp_obj* d = LP_OBJ(1);
    lp_obj* r = lp_list(lp);

    int i = 0;
    while ((i=_lp_str_index(v,i,d))!=-1) {
        _lp_list_append(lp,r->list,lp_string_sub(lp,v,0,i));
        i += d->string.len;
    }
    _lp_list_append(lp,r->list,lp_string_sub(lp,v,0,v->string.len));
    return r;
}


lp_obj* lpf_find(LP) {
    lp_obj* s = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
    return lp_number_from_int(lp, _lp_str_index(s,0,v));
}

lp_obj* lpf_str_index(LP) {
    lp_obj* s = LP_OBJ(0);
    lp_obj* v = LP_OBJ(1);
    int n = _lp_str_index(s,0,v);
    if (n >= 0) { return lp_number_from_int(lp, n); }
    lp_raise(0,lp_string(lp, "(lp_str_index) ValueError: substring not found"));
}

lp_obj* lpf_str2(LP) {
    lp_obj* v = LP_OBJ(0);
    return lp_str(lp,v);
}

lp_obj* lpf_chr(LP) {
	int v = LP_INTEGER(0);
    return lp_string_n(lp, lp->chars[(unsigned char)v],1);
}
lp_obj* lpf_ord(LP) {
    lp_obj* s = LP_STR(0);
    if (s->string.len != 1) {
        lp_raise(0,lp_string(lp, "(lp_ord) TypeError: ord() expected a character"));
    }
    return lp_number_from_int(lp, (unsigned char)s->string.val[0]);
}

lp_obj* lpf_strip(LP) {
    lp_obj* o = LP_TYPE(0, LP_STRING);
    char const *v = o->string.val; int l = o->string.len;
    int i; int a = l, b = 0;
    lp_obj* r;
    char *s;
    for (i=0; i<l; i++) {
        if (v[i] != ' ' && v[i] != '\n' && v[i] != '\t' && v[i] != '\r') {
            a = _lp_min(a,i); b = _lp_max(b,i+1);
        }
    }
    if ((b-a) < 0) { return lp_string(lp, ""); }
    r = lp_string_t(lp,b-a);
    s = r->string.info->s;
    memcpy(s,v+a,b-a);
    return r;
}

lp_obj* lpf_replace(LP) {
    lp_obj* s = LP_OBJ(0);
    lp_obj* k = LP_OBJ(1);
    lp_obj* v = LP_OBJ(2);
    int i = 0,n = 0;
    int c;
    int l;
    lp_obj* rr;
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

