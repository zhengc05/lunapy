#pragma once

/* builtins */
#define LP_META_BEGIN(self,name) \
    if (self->dict.dtype == 2) { \
        lp_obj* meta; if (lp_lookupx(lp,self,name,&meta)) {

#define LP_META_END \
        } \
    }


lp_obj* lpf_print(LP);
lp_obj* lpf_bind(LP);
lp_obj* lpf_min(LP);
lp_obj* lpf_max(LP);
lp_obj* lpf_copy(LP);
lp_obj* lpf_len(LP);
lp_obj* lpf_assert(LP);
lp_obj* lpf_range(LP);
lp_obj* lpf_system(LP);
lp_obj* lpf_istype(LP);
lp_obj* lpf_float(LP);
lp_obj* lpf_save(LP);
lp_obj* lpf_load(LP);
lp_obj* lpf_fpack(LP);
lp_obj* lpf_abs(LP);
lp_obj* lpf_int(LP);
lp_obj* lpf_round(LP);
lp_obj* lpf_exists(LP);
lp_obj* lpf_mtime(LP);
lp_obj* lpf_setmeta(LP);
lp_obj* lpf_getmeta(LP);
lp_obj* lpf_object(LP);
lp_obj* lpf_object_new(LP);
lp_obj* lpf_object_call(LP);
lp_obj* lpf_getraw(LP);
lp_obj* lpf_class(LP);
lp_obj* lpf_builtins_bool(LP);


/* list */
_lp_list *_lp_list_new(LP);
void _lp_list_free(LP, _lp_list *self);
lp_obj* _lp_list_copy(LP, lp_obj* rr);
lp_obj* _lp_list_pop(LP,_lp_list *self, int n, const char *error);
void _lp_list_append(LP,_lp_list *self, lp_obj* v);
void _lp_list_appendx(LP,_lp_list *self, lp_obj* v);
void _lp_list_set(LP,_lp_list *self,int k, lp_obj* v, const char *error);
int _lp_list_find(LP,_lp_list *self, lp_obj* v);
void _lp_list_insert(LP,_lp_list *self, int n, lp_obj* v);
lp_obj* lpf_index(LP);
lp_obj* lpf_append(LP);
lp_obj* lpf_pop(LP);
lp_obj* lpf_insert(LP);
lp_obj* lpf_extend(LP);
lp_obj* lpf_sort(LP);

/* dict */
void _lp_dict_free(LP, _lp_dict *self);
void _lp_dict_set(LP,_lp_dict *self,lp_obj* k, lp_obj* v);
lp_obj* _lp_dict_get(LP,_lp_dict *self,lp_obj* k, const char *error);
int _lp_dict_find(LP,_lp_dict *self,lp_obj* k);
int _lp_dict_next(LP,_lp_dict *self);
void _lp_dict_del(LP,_lp_dict *self,lp_obj* k, const char *error);
lp_obj* lpf_merge(LP);

/* string */
int _lp_str_index(lp_obj* s, int i, lp_obj* k);
int _lp_str_cmp(lp_obj* s, const char* k);
lp_obj* lpf_join(LP);
lp_obj* lpf_split(LP);
lp_obj* lpf_find(LP);
lp_obj* lpf_str_index(LP);
lp_obj* lpf_str2(LP);
lp_obj* lpf_chr(LP);
lp_obj* lpf_ord(LP);
lp_obj* lpf_strip(LP);
lp_obj* lpf_replace(LP);

/* gc */
void lp_grey(LP, lp_obj* v);
void lp_gc_init(LP);
void lp_delete(LP,lp_obj* v);
void lp_full(LP);
void lp_gc_deinit(LP);
lp_obj* lp_track(LP, lp_obj* v);
