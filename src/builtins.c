#include "lp.h"
#include "lp_internal.h"

/* File: Builtins
 * Builtin tinypy functions.
 */

lp_obj* lpf_print(LP) {
    int n = 0;
    lp_obj* e;
    LP_LOOP(0, e)
        if (n) { printf(" "); }
        lp_echo(lp,e);
        n += 1;
    LP_END;
    printf("\n");
	RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lpf_bind(LP) {
    lp_obj* r = LP_TYPE(0,LP_FNC);
    lp_obj* self = LP_OBJ(1);
    return lp_fnc_new(lp,
        r->fnc.ftype|2,r->fnc.cfnc,r->fnc.info->code,
        self,r->fnc.info->globals);
}

lp_obj* lpf_min(LP) {
    lp_obj* r = LP_OBJ(0);
    lp_obj* e;
    LP_LOOP(1, e)
        if (lp_cmp(lp,r,e) > 0) { r = e; }
    LP_END;
	RETURN_LP_OBJ(r);
}

lp_obj* lpf_max(LP) {
    lp_obj* r = LP_OBJ(0);
    lp_obj* e;
    LP_LOOP(1, e)
        if (lp_cmp(lp,r,e) < 0) { r = e; }
    LP_END;
	RETURN_LP_OBJ(r);
}

lp_obj* lpf_copy(LP) {
    lp_obj* r = LP_OBJ(0);
    int type = r->type;
    if (type == LP_LIST) {
        return lp_list_copy(lp,r);
    } else if (type == LP_DICT) {
        return lp_dict_copy(lp,r);
    }
    lp_raise(0,lp_string(lp, "(lp_copy) TypeError: ?"));
}


lp_obj* lpf_len(LP) {
    lp_obj* e = LP_OBJ(0);
    return lp_len(lp,e);
}

lp_obj* lpf_assert(LP) {
    int a = LP_NUM(0);
    if (a) { RETURN_LP_OBJ(lp->lp_None); }
    lp_raise(0,lp_string(lp, "(lp_assert) AssertionError"));
}

lp_obj* lpf_range(LP) {
    int a,b,c,i;
    lp_obj* r = lp_list(lp);
    switch (lp->params->list->len) {
        case 1: a = 0; b = LP_INTEGER(0); c = 1; break;
        case 2:
        case 3: a = LP_INTEGER(0); b = LP_INTEGER(1); c = LP_INTEGER_DEFAULT(2, 1); break;
        default: return r;
    }
    if (c != 0) {
        for (i=a; (c>0) ? i<b : i>b; i+=c) {
			lp_obj* t= lp_number_from_int(lp, i);
            _lp_list_appendx(lp,r->list,t);
        }
    }
    return r;
}

/* Function: lp_system
 *
 * The system builtin. A grave security flaw. If your version of tinypy
 * enables this, you better remove it before deploying your app :P
 */
lp_obj* lpf_system(LP) {
    char s[LP_CSTR_LEN]; lp_cstr(lp,LP_STR(0),s,LP_CSTR_LEN);
    int r = system(s);
    return lp_number_from_int(lp, r);
}

lp_obj* lpf_istype(LP) {
    lp_obj* v = LP_OBJ(0);
    lp_obj* t = LP_STR(1);
    if (lp_cmp(lp, t, lp_string(lp, "string")) == 0) { return lp_number_from_int(lp, v->type == LP_STRING); }
    if (lp_cmp(lp, t, lp_string(lp, "list")) == 0) { return lp_number_from_int(lp, v->type == LP_LIST); }
    if (lp_cmp(lp, t, lp_string(lp, "dict")) == 0) { return lp_number_from_int(lp, v->type == LP_DICT); }
	if (lp_cmp(lp, t, lp_string(lp, "int")) == 0) { return lp_number_from_int(lp, v->type == LP_INT); }
    if (lp_cmp(lp, t, lp_string(lp, "float")) == 0) { return lp_number_from_int(lp, v->type == LP_DOUBLE); }
	if (lp_cmp(lp, t, lp_string(lp, "number")) == 0) { return lp_number_from_int(lp, v->type == LP_INT || v->type == LP_DOUBLE); }
    if (lp_cmp(lp, t, lp_string(lp, "fnc")) == 0) { return lp_number_from_int(lp, v->type == LP_FNC && (v->fnc.ftype&2) == 0); }
    if (lp_cmp(lp, t, lp_string(lp, "method")) == 0) { return lp_number_from_int(lp, v->type == LP_FNC && (v->fnc.ftype&2) != 0); }
    lp_raise(0, lp_string(lp, "(is_type) TypeError: ?"));
}


lp_obj* lpf_float(LP) {
    lp_obj* v = LP_OBJ(0);
    int ord = LP_INTEGER_DEFAULT(1,0);
    int type = v->type;
	if (type == LP_DOUBLE)
	{
		RETURN_LP_OBJ(v);
	}
    if (type == LP_INT) {
		return lp_number_from_double(lp, v->integer);
	}
    if (type == LP_STRING && v->string.len < 32) {
        char s[32]; memset(s,0,v->string.len+1);
        memcpy(s,v->string.val,v->string.len);
        if (strchr(s,'.')) { return lp_number_from_double(lp, atof(s)); }
        return(lp_number_from_double(lp, strtol(s,0,ord)));
    }
    lp_raise(0,lp_string(lp, "(lp_float) TypeError: ?"));
}


lp_obj* lpf_save(LP) {
    char fname[256]; lp_cstr(lp,LP_STR(0),fname,256);
    lp_obj* v = LP_OBJ(1);
    FILE *f;
    f = fopen(fname,"wb");
    if (!f) { lp_raise(0,lp_string(lp, "(lp_save) IOError: ?")); }
    fwrite(v->string.val,v->string.len,1,f);
    fclose(f);
	RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lpf_load(LP) {
    FILE *f;
    long l;
    lp_obj* r;
    char *s;
    char fname[256]; lp_cstr(lp,LP_STR(0),fname,256);
    struct stat stbuf;
    stat(fname, &stbuf);
    l = stbuf.st_size;
    f = fopen(fname,"rb");
    if (!f) {
        lp_raise(0,lp_string(lp, "(lp_load) IOError: ?"));
    }
    r = lp_string_t(lp,l);
    s = r->string.info->s;
    fread(s,1,l,f);
/*    if (rr !=l) { printf("hmmn: %d %d\n",rr,(int)l); }*/
    fclose(f);
    return r;
}


lp_obj* lpf_fpack(LP) {
    double v = LP_NUM(0);
    lp_obj* r = lp_string_t(lp,sizeof(double));
    *(double*)r->string.val = v;
    return r;
}

lp_obj* lpf_abs(LP) {
	lp_obj* t = lpf_float(lp);
	double n = t->doublen;
	LP_OBJ_DEC(t);
    return lp_number_from_double(lp, fabs(n));
}
lp_obj* lpf_int(LP) {
	lp_obj* t = lpf_float(lp);
	double n = t->doublen;
	LP_OBJ_DEC(t);
    return lp_number_from_int(lp, (int)n);
}
double _roundf(double v) {
	double av = fabs(v); double iv = (long)av;
    av = (av-iv < 0.5?iv:iv+1);
    return (v<0?-av:av);
}
lp_obj* lpf_round(LP) {
	lp_obj* t = lpf_float(lp);
	double n = t->doublen;
	LP_OBJ_DEC(t);
    return lp_number_from_double(lp, _roundf(n));
}

lp_obj* lpf_exists(LP) {
    char fname[LP_CSTR_LEN]; lp_cstr(lp,LP_STR(0),fname,LP_CSTR_LEN);
    struct stat stbuf;
    return lp_number_from_int(lp, !stat(fname,&stbuf));
}
lp_obj* lpf_mtime(LP) {
    char fname[LP_CSTR_LEN]; lp_cstr(lp,LP_STR(0),fname,LP_CSTR_LEN);
    struct stat stbuf;
    if (!stat(fname,&stbuf)) { return lp_number_from_double(lp, stbuf.st_mtime); }
    lp_raise(0,lp_string(lp, "(lp_mtime) IOError: ?"));
}

int lp_lookup_(LP,lp_obj* self, lp_obj* k, lp_obj **meta, int depth) {
    int n = _lp_dict_find(lp,self->dict.val,k);
    if (n != -1) {
        *meta = self->dict.val->items[n].val;
		LP_OBJ_INC(*meta);
        return 1;
    }
    depth--; if (!depth) { lp_raise(0,lp_string(lp, "(lp_lookup) RuntimeError: maximum lookup depth exceeded")); }
    if (self->dict.dtype && self->dict.val->meta && self->dict.val->meta->type == LP_DICT && lp_lookup_(lp,self->dict.val->meta,k,meta,depth)) {
        if (self->dict.dtype == 2 && (*meta)->type == LP_FNC) {
			lp_obj* t = *meta;
            *meta = lp_fnc_new(lp,t->fnc.ftype|2,
                t->fnc.cfnc,t->fnc.info->code,
                self,t->fnc.info->globals);
			LP_OBJ_DEC(t);
        }
        return 1;
    }
    return 0;
}

int lp_lookup(LP,lp_obj* self, lp_obj* k, lp_obj **meta) {
    return lp_lookup_(lp,self,k,meta,8);
}

int lp_lookupx(LP, lp_obj* self, const char* k, lp_obj **meta) {
	lp_obj *name = lp_string(lp, k);
	int r = lp_lookup_(lp, self, name, meta, 8);
	LP_OBJ_DEC(name);
	return r;
}

/* Function: lp_setmeta
 * Set a "dict's meta".
 *
 * This is a builtin function, so you need to use <lp_params> to provide the
 * parameters.
 *
 * In tinypy, each dictionary can have a so-called "meta" dictionary attached
 * to it. When dictionary attributes are accessed, but not present in the
 * dictionary, they instead are looked up in the meta dictionary. To get the
 * raw dictionary, you can use <lp_getraw>.
 *
 * This function is particulary useful for objects and classes, which are just
 * special dictionaries created with <lp_object> and <lp_class>. There you can
 * use lp_setmeta to change the class of the object or parent class of a class.
 *
 * Parameters:
 * self - The dictionary for which to set a meta.
 * meta - The meta dictionary.
 *
 * Returns:
 * None
 */
lp_obj* lpf_setmeta(LP) {
    lp_obj* self = LP_TYPE(0, LP_DICT);
    lp_obj* meta = LP_TYPE(1, LP_DICT);
    self->dict.val->meta = meta;
	LP_OBJ_INC(meta);
    RETURN_LP_OBJ(lp->lp_None);
}

lp_obj* lpf_getmeta(LP) {
    lp_obj* self = LP_TYPE(0, LP_DICT);
	RETURN_LP_OBJ(self->dict.val->meta);
}

/* Function: lp_object
 * Creates a new object.
 *
 * Returns:
 * The newly created object. The object initially has no parent class, use
 * <lp_setmeta> to set a class. Also see <lp_object_new>.
 */
lp_obj* lp_object(LP) {
    lp_obj* self = lp_dict(lp);
    self->dict.dtype = 2;
    return self;
}

lp_obj* lpf_object_new(LP) {
    lp_obj* klass = _lp_list_pop(lp, lp->params->list, 0, "object");
    lp_obj* self = lp_object(lp);
	lp_obj* r;
    self->dict.val->meta = klass;
    LP_META_BEGIN(self, "__init__");
        r = lp_call(lp,meta);
		LP_OBJ_DEC(meta);
		if (!r) return 0;
		LP_OBJ_DEC(r);
    LP_META_END;
    return self;
}

lp_obj* lpf_object_call(LP) {
    lp_obj* self;
    if (lp->params->list->len) {
        self = LP_TYPE(0, LP_DICT);
        self->dict.dtype = 2;
    } else {
        self = lp_object(lp);
    }
    return self;
}

/* Function: lp_getraw
 * Retrieve the raw dict of a dict.
 *
 * This builtin retrieves one dict parameter from tinypy, and returns its raw
 * dict. This is very useful when implementing your own __get__ and __set__
 * functions, as it allows you to directly access the attributes stored in the
 * dict.
 */
lp_obj* lpf_getraw(LP) {
    lp_obj* self = LP_TYPE(0, LP_DICT);
	if (!self) return 0;
    self->dict.dtype = 0;
	LP_OBJ_INC(self);
    return self;
}

/* Function: lp_class
 * Creates a new base class.
 *
 * Parameters:
 * none
 *
 * Returns:
 * A new, empty class (derived from tinypy's builtin "object" class).
 */
lp_obj* lp_class(LP) {
    lp_obj* klass = lp_dict(lp);
	lp_obj* k = lp_string(lp, "object");
    lp_obj* f = lp_get(lp,lp->builtins,k);
	klass->dict.val->meta = f;
	LP_OBJ_DEC(k);
    return klass;
}

/* Function: lp_builtins_bool
 * Coerces any value to a boolean.
 */
lp_obj* lpf_builtins_bool(LP) {
    lp_obj* v = LP_OBJ(0);
    return (lp_number_from_int(lp, lp_bool(lp, v)));
}
