/* File: General
 * Things defined in lp.h.
 */
#ifndef LP_H
#define LP_H

#include <setjmp.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#ifdef __GNUC__
#define lp_inline __inline__
#endif

#ifdef _MSC_VER
#define lp_inline __inline
#endif

#ifndef lp_inline
#error "Unsuported compiler"
#endif

/*  #define lp_malloc(x) calloc((x),1)
    #define lp_realloc(x,y) realloc(x,y)
    #define lp_free(x) free(x) */

/* #include <gc/gc.h>
   #define lp_malloc(x) GC_MALLOC(x)
   #define lp_realloc(x,y) GC_REALLOC(x,y)
   #define lp_free(x)*/

enum {
    LP_NONE, LP_INT, LP_DOUBLE, LP_STRING,LP_DICT,
    LP_LIST,LP_FNC,LP_DATA,
};

typedef struct lp_string_ {
    struct _lp_string *info;
    char const *val;
    int len;
} lp_string_;
typedef struct lp_dict_ {
    struct _lp_dict *val;
    int dtype;
} lp_dict_;
typedef struct lp_fnc_ {
    struct _lp_fnc *info;
    int ftype;
    void *cfnc;
} lp_fnc_;
typedef struct lp_data_ {
	void(*free_fun)(LP, lp_obj);
    void *val;
    int magic;
} lp_data_;

struct _lp_list;

/* Type: lp_obj
 * Lunapy's object representation.
 * 
 * Every object in tinypy is of this type in the C API.
 *
 * Fields:
 * type - This determines what kind of objects it is. It is either LP_NONE, in
 *        which case this is the none type and no other fields can be accessed.
 *        Or it has one of the values listed below, and the corresponding
 *        fields can be accessed.
 * integer - LP_INTEGER
 * double - LP_DOUBLE
 * string - LP_STRING
 * string.val - A pointer to the string data.
 * string.len - Length in bytes of the string data.
 * dict - LP_DICT
 * list - LP_LIST
 * fnc - LP_FNC
 * data - LP_DATA
 * data.val - The user-provided data pointer.
 * data.magic - The user-provided magic number for identifying the data type.
 */
typedef struct lp_obj {
    int type;
	int ref;
	struct lp_obj* prev;
	struct lp_obj* next;
	void* pool;
	union {
		int integer;
		double doublen;
		lp_string_ string;
		struct _lp_list *list;
		lp_dict_ dict;
		lp_fnc_ fnc;
		lp_data_ data;
	};
} lp_obj;

typedef struct _lp_string {
    int ref;
    int len;
	void *pool;
	int index;
    char s[];
} _lp_string;
typedef struct _lp_list {
	struct _lp_list *prev;
	struct _lp_list *next;
	void *pool;
	lp_obj **items;
	int hold;
	int len;
	int alloc;
	void *item_pool;
	int item_index;
} _lp_list;
typedef struct lp_item {
    int used;
    int hash;
    lp_obj* key;
    lp_obj* val;
} lp_item;
typedef struct _lp_dict {
	struct _lp_dict *prev;
	struct _lp_dict *next;
	void *pool;
    lp_item *items;
    int len;
    int alloc;
    int cur;
    int mask;
    int used;
	int hold;
    lp_obj* meta;
	void *item_pool;
	int item_index;
} _lp_dict;
typedef struct _lp_fnc {
	struct _lp_fnc *prev;
	struct _lp_fnc *next;
	void *pool;
    lp_obj* self;
    lp_obj* globals;
    lp_obj* code;
	int hold;
} _lp_fnc;


typedef union lp_code {
    unsigned char i;
    struct { unsigned char i,a,b,c; } regs;
    struct { char val[4]; } string;
    struct { float val; } number;
} lp_code;

typedef struct lp_frame_ {
/*    lp_code *codes; */
    lp_obj* code;
    lp_code *cur;
    lp_code *jmp;
    lp_obj **regs;
    lp_obj **ret_dest;
    lp_obj* fname;
    lp_obj* name;
    lp_obj* line;
    lp_obj* globals;
    int lineno;
    int cregs;
} lp_frame_;

#define LP_GCMAX 4096
#define LP_FRAMES 256
#define LP_REGS_EXTRA 2
/* #define LP_REGS_PER_FRAME 256*/
#define LP_REGS 16384

#define OBJ_SIZE 1024
#define DICT_SIZE 128
#define LIST_SIZE 128
#define FUNC_SIZE 128

struct LpObjPool
{
	lp_obj obj[OBJ_SIZE];
	lp_obj *head;
	lp_obj *tail;
	struct LpObjPool* next;
};

struct LpStringPool
{
	int *bit_map;
	int total_size;
	struct LpStringPool* next;
	int not_used;
	char mem[];
};

struct LpItemPool
{
	int *bit_map;
	int total_size;
	struct LpItemPool* next;
	int not_used;
	lp_item mem[];
};

struct LpObjArrayPool
{
	int *bit_map;
	int total_size;
	struct LpObjArrayPool* next;
	int not_used;
	lp_obj* mem[];
};

struct LpDictPool
{
	_lp_dict obj[DICT_SIZE];
	_lp_dict *head;
	_lp_dict *tail;
	struct LpDictPool* next;
};

struct LpListPool
{
	_lp_list obj[LIST_SIZE];
	_lp_list *head;
	_lp_list *tail;
	struct LpListPool* next;
};

struct LpFunPool
{
	_lp_fnc obj[FUNC_SIZE];
	_lp_fnc *head;
	_lp_fnc *tail;
	struct LpFunPool* next;
};

/* Type: lp_vm
 * Representation of a tinypy virtual machine instance.
 * 
 * A new lp_vm struct is created with <lp_init>, and will be passed to most
 * tinypy functions as first parameter. It contains all the data associated
 * with an instance of a tinypy virtual machine - so it is easy to have
 * multiple instances running at the same time. When you want to free up all
 * memory used by an instance, call <lp_deinit>.
 * 
 * Fields:
 * These fields are currently documented: 
 * 
 * builtins - A dictionary containing all builtin objects.
 * modules - A dictionary with all loaded modules.
 * params - A list of parameters for the current function call.
 * frames - A list of all call frames.
 * cur - The index of the currently executing call frame.
 * frames[n].globals - A dictionary of global sybmols in callframe n.
 */
typedef struct lp_vm {
    lp_obj* builtins;
	lp_obj* path;
    lp_obj* modules;
    lp_frame_ frames[LP_FRAMES];
    lp_obj* _params;
    lp_obj* params;
    lp_obj* _regs;
    lp_obj **regs;
    lp_obj* root;
	lp_obj* lp_None;
	lp_obj* lp_True;
	lp_obj* lp_False;
    lp_obj* ex;
	lp_obj* oldex;
    char chars[256][2];
    int cur;
	struct LpObjPool *obj_pool;
	struct LpStringPool *string_pool;
	struct LpItemPool *item_pool;
	struct LpObjArrayPool *obj_array_pool;
	struct LpDictPool *dict_pool;
	struct LpListPool *list_pool;
	struct LpFunPool *func_pool;
    int steps;
    /* sandbox */
    clock_t clocks;
    double time_elapsed;
    double time_limit;
    unsigned long mem_limit;
    unsigned long mem_used;
    int mem_exceeded;
} lp_vm;

#define LP lp_vm *lp

#define LP_OBJ_INC(obj) if(obj) {(obj)->ref++;}
#define LP_OBJ_DEC(obj) lp_obj_dec(lp, obj)
#define RETURN_LP_OBJ(obj) do { LP_OBJ_INC(obj)return obj; } while(0);
#define RETURN_LP_NONE do { LP_OBJ_INC(lp->lp_None)return lp->lp_None; } while(0);

/* lp */
lp_obj* lp_obj_new(LP, int type);
void lp_obj_dec(LP, lp_obj* obj);
lp_obj* lp_none(LP);

lp_item* lp_item_malloc(LP, int count, void** item_pool, int* item_index);
void lp_item_release(LP, int count, void* item_pool, int item_index);
lp_obj** lp_obj_array_malloc(LP, int count, void** item_pool, int* item_index);
void lp_obj_array_release(LP, int count, void* item_pool, int item_index);
_lp_dict* lp_dict_new(LP);
void lp_dict_release(LP, _lp_dict* dict);
_lp_list* lp_list_new(LP);
void lp_list_release(LP, _lp_list* list);
_lp_string* lp_string_new(LP, int len);
void lp_string_release(LP, _lp_string* string);
_lp_fnc* lp_fnc_malloc(LP);
void lp_fnc_free(LP, _lp_fnc* fnc);
void lp_print_object_pool(LP);

/* builtins */
int lp_lookup(LP, lp_obj* self, lp_obj* k, lp_obj **meta);
int lp_lookupx(LP, lp_obj* self, const char* k, lp_obj **meta);
lp_obj* lp_object(LP);
lp_obj* lp_class(LP);

/* dict */
int lp_hash(LP, lp_obj* v);
lp_obj* lp_dict_copy(LP, lp_obj* rr);
lp_obj* lp_merge(LP);
lp_obj* lp_dict(LP);
lp_obj* lp_dict_n(LP, int n, lp_obj** argv);

/* list */
lp_obj* lp_list_copy(LP, lp_obj* rr);
lp_obj* lp_list_nt(LP);
lp_obj* lp_list(LP);
lp_obj* lp_list_n(LP, int n, lp_obj **argv);
lp_obj* _lp_list_iget(LP, _lp_list *self, int k);
lp_obj* _lp_list_get(LP, _lp_list *self, int k, const char *error);

/* misc */
lp_obj* lp_tcall(LP, lp_obj* fnc);
lp_obj* lp_def(LP, lp_obj* code, lp_obj* g);
lp_obj* lp_fnc_new(LP, int t, void *v, lp_obj* c, lp_obj* s, lp_obj* g);
lp_obj* lp_fnc(LP, lp_obj* v(LP));
lp_obj* lp_method(LP, lp_obj* self, lp_obj* v(LP));
lp_obj* lp_data(LP, int magic, void *v);
lp_obj* lp_params(LP);
void lp_params_n(LP, int n, lp_obj* argv[]);
void lp_params_v(LP, int n, ...);
void lp_params_v_x(LP, int n, ...);

/* ops */
lp_obj* lp_str(LP, lp_obj* self);
int lp_bool(LP, lp_obj* v);
lp_obj* lp_has(LP, lp_obj* self, lp_obj* k);
void lp_del(LP, lp_obj* self, lp_obj* k);
lp_obj* lp_iter(LP, lp_obj* self, lp_obj* k);
lp_obj* lp_get(LP, lp_obj* self, lp_obj* k);
lp_obj* lp_getk(LP, lp_obj* self, lp_obj* k);
int lp_iget(LP, lp_obj **r, lp_obj* self, lp_obj* k);
int lp_igetk(LP, lp_obj **r, lp_obj* self, lp_obj* k);
void lp_set(LP, lp_obj* self, lp_obj* k, lp_obj* v);
void lp_setk(LP, lp_obj* self, lp_obj* k, lp_obj* v);
void lp_setv(LP, lp_obj* self, lp_obj* k, lp_obj* v);
void lp_setkv(LP, lp_obj* self, lp_obj* k, lp_obj* v);
lp_obj* lp_add(LP, lp_obj* a, lp_obj* b);
lp_obj* lp_mul(LP, lp_obj* a, lp_obj* b);
lp_obj* lp_len(LP, lp_obj* self);
int lp_lenx(LP, lp_obj* self);
int lp_cmp(LP, lp_obj* a, lp_obj* b);
int lp_cmps(LP, lp_obj* a, const char* b);
lp_obj* lp_sub(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_div(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_pow(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_bitwise_and(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_bitwise_or(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_bitwise_not(LP, lp_obj* _a);
lp_obj* lp_bitwise_xor(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_mod(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_lsh(LP, lp_obj* _a, lp_obj* _b);
lp_obj* lp_rsh(LP, lp_obj* _a, lp_obj* _b);

/* string */
lp_obj* lp_string_t(LP, int n);
lp_obj* lp_string_copy(LP, const char *s, int n);
lp_obj* lp_string_sub(LP, lp_obj* s, int a, int b);
lp_obj* lp_printf(LP, char const *fmt, ...);

/* vm */
void lp_deinit(LP);
void lp_frame(LP, lp_obj* globals, lp_obj* code, lp_obj **ret_dest);
void lp_print_stack(LP);
lp_obj* lp_call(LP, lp_obj* self);
void lp_return(LP, lp_obj* v);
lp_obj* lp_ez_call(LP, const char *mod, const char *fnc);
lp_obj* lp_import(LP, lp_obj* fname, lp_obj* name, lp_obj* code);
lp_obj* lp_import_(LP, const char * fname, const char * name, void *codes, int len);
lp_obj* lp_compile(LP, lp_obj* text, lp_obj* fname);
lp_obj* lp_exec(LP, lp_obj* code, lp_obj* globals);
void lp_args(LP, int argc, char *argv[]);
lp_obj* lp_main(LP, char *fname, void *code, int len);
lp_obj* lp_eval(LP, const char *text, lp_obj* globals);
lp_vm *lp_init(int argc, char *argv[]);
lp_obj* lp_disasm(LP, lp_obj* code);



/* __func__ __VA_ARGS__ __FILE__ __LINE__ */

/* Function: lp_raise
 * Macro to raise an exception.
 * 
 * This macro will return from the current function returning "r". The
 * remaining parameters are used to format the exception message.
 */
/*
#define lp_raise(r,fmt,...) { \
    _lp_raise(lp,lp_printf(lp,fmt,__VA_ARGS__)); \
    return r; \
}
*/
#define lp_raise(r,v) { \
	lp_obj_dec(lp, lp->ex); \
    lp->ex = v; \
    return r; \
}

/* Function: lp_string
 * Creates a new string object from a C string.
 * 
 * Given a pointer to a C string, creates a tinypy object representing the
 * same string.
 * 
 * *Note* Only a reference to the string will be kept by tinypy, so make sure
 * it does not go out of scope, and don't de-allocate it. Also be aware that
 * tinypy will not delete the string for you. In many cases, it is best to
 * use <lp_string_t> or <lp_string_slice> to create a string where tinypy
 * manages storage for you.
 */
lp_inline lp_obj* lp_string(LP, char const *v) {
    lp_obj* val = lp_obj_new(lp, LP_STRING);
    lp_string_ s = {0, v, 0};
    s.len = strlen(v);
    val->string = s;
    return val;
}

lp_obj* lp_string_t(LP, int n);
lp_obj* lp_string_copy(LP, const char *s, int n);

#define LP_CSTR_LEN 256

lp_inline void lp_cstr(LP,lp_obj* v, char *s, int l) {
    if (v->type != LP_STRING) { 
        lp_raise(,lp_string(lp, "(lp_cstr) TypeError: value not a string"));
    }
    if (v->string.len >= l) {
        lp_raise(,lp_string(lp, "(lp_cstr) TypeError: value too long"));
    }
    memset(s,0,l);
    memcpy(s,v->string.val,v->string.len);
}


#define LP_OBJ(n) (_lp_list_iget(lp,lp->params->list,n))
lp_inline lp_obj* lp_type(LP,int t,lp_obj* v) {
    if (v->type != t) { lp_raise(0,lp_string(lp, "(lp_type) TypeError: unexpected type")); }
    return v;
}

lp_inline int lp_is_number(lp_obj* v) {
	return v->type == LP_INT || v->type == LP_DOUBLE;
}

lp_inline double lp_type_number(LP, lp_obj* v) {
	double n;
	if (v->type != LP_INT && v->type != LP_DOUBLE) { lp_raise(0, lp_string(lp, "(lp_type) TypeError: unexpected type")); }
	if (v->type == LP_INT) n = v->integer;
	else n = v->doublen;
	return n;
}

#define LP_NO_LIMIT 0
#define LP_TYPE(n,t) lp_type(lp,t,LP_OBJ(n))
#define LP_NUM(n) (lp_type_number(lp,LP_OBJ(n)))
#define LP_INTEGER(n) (LP_TYPE(n,LP_INT)->integer)
/* #define LP_STR() (LP_CSTR(LP_TYPE(LP_STRING))) */
#define LP_STR(n) (LP_TYPE(n,LP_STRING))
#define LP_INTEGER_DEFAULT(n,d) (n<lp->params->list->len?lp_type_number(lp,LP_OBJ(n)):(d))
#define LP_DEFAULT(n,o) (n<lp->params->list->len?LP_OBJ(n):(o))

/* Macro: LP_LOOP
 * Macro to iterate over all remaining arguments.
 *
 * If you have a function which takes a variable number of arguments, you can
 * iterate through all remaining arguments for example like this:
 *
 * > lp_obj *my_func(lp_vm *lp)
 * > {
 * >     // We retrieve the first argument like normal.
 * >     lp_obj first = LP_OBJ();
 * >     // Then we iterate over the remaining arguments.
 * >     lp_obj arg;
 * >     LP_LOOP(start, arg)
 * >         // do something with arg
 * >     LP_END
 * > }
 */
#define LP_LOOP(s,e) \
    int __l = lp->params->list->len; \
    int __i; for (__i=s; __i<__l; __i++) { \
    (e) = _lp_list_get(lp,lp->params->list,__i,"LP_LOOP");
#define LP_END \
    }

lp_obj* _lp_list_get(LP,_lp_list *self,int k,const char *error);

lp_inline int _lp_min(int a, int b) { return (a<b?a:b); }
lp_inline int _lp_max(int a, int b) { return (a>b?a:b); }
lp_inline int _lp_sign(double v) { return (v<0?-1:(v>0?1:0)); }

/* Function: lp_number
 * Creates a new numeric object.
 */
lp_inline lp_obj* lp_number_from_double(LP, double v) {
    lp_obj* val = lp_obj_new(lp, LP_DOUBLE);
    val->doublen = v;
    return val;
}

lp_inline lp_obj* lp_number_from_int(LP, int v) {
	lp_obj* val = lp_obj_new(lp, LP_INT);
	val->integer = v;
	return val;
}

lp_inline void lp_echo(LP,lp_obj* e) {
    e = lp_str(lp,e);
    fwrite(e->string.val,1,e->string.len,stdout);
}

/* Function: lp_string_n
 * Creates a new string object from a partial C string.
 * 
 * Like <lp_string>, but you specify how many bytes of the given C string to
 * use for the string object. The *note* also applies for this function, as the
 * string reference and length are kept, but no actual substring is stored.
 */
lp_inline lp_obj* lp_string_n(LP, char const *v,int n) {
    lp_obj* val = lp_obj_new(lp, LP_STRING);
    lp_string_ s = {0,v,n};
    val->string = s;
    return val;
}



const char* compile(const char* fname, char* code, int* size, int *res);

#endif
