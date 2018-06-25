#include "lp.h"
#include "lp_internal.h"

/* File: Miscellaneous
 * Various functions to help interface tinypy.
 */

lp_obj _lp_dcall(LP,lp_obj fnc(LP)) {
    return fnc(lp);
}
lp_obj lp_tcall(LP,lp_obj fnc) {
	lp_fnc* _fnc = tp_obj_to_fnc(lp, fnc);
    if (_fnc->ftype&2) {
        _lp_list_insert(lp,tp_obj_to_list(lp->params),0,_fnc->self);
    }
    return _lp_dcall(lp,(lp_obj *(*)(lp_vm *))_fnc->cfnc);
}

lp_obj lp_fnc_new(LP,int t, void *v, lp_obj c,lp_obj s, lp_obj g) {
    lp_fnc *info = lp_fnc_malloc(lp);
	info->ftype = t;
    info->code = c;
    info->self = s;
    info->globals = g;
	info->cfnc = v;
	LP_OBJ_INC(c);
	LP_OBJ_INC(s);
	LP_OBJ_INC(g);
    return ((unsigned int)info) | LP_FNC;
}

lp_obj lp_def(LP,lp_obj code, lp_obj g) {
    lp_obj r = lp_fnc_new(lp,1,0,code,lp->lp_None,g);
    return r;
}

/* Function: lp_fnc
 * Creates a new tinypy function object.
 * 
 * This is how you can create a tinypy function object which, when called in
 * the script, calls the provided C function.
 */
lp_obj lp_fnc_n(LP,lp_obj v(LP)) {
    return lp_fnc_new(lp,0,v,lp->lp_None,lp->lp_None,lp->lp_None);
}

lp_obj lp_method(LP,lp_obj self,lp_obj v(LP)) {
    return lp_fnc_new(lp,2,v,lp->lp_None,self,lp->lp_None);
}

/* Function: lp_data
 * Creates a new data object.
 * 
 * Parameters:
 * magic - An integer number associated with the data type. This can be used
 *         to check the type of data objects.
 * v     - A pointer to user data. Only the pointer is stored in the object,
 *         you keep all responsibility for the data it points to.
 *
 * 
 * Returns:
 * The new data object.
 * 
 * Public fields:
 * The following fields can be access in a data object:
 * 
 * magic      - An integer number stored in the object.
 * val        - The data pointer of the object.
 * info->free - If not NULL, a callback function called when the object gets
 *              destroyed.
 * 
 * Example:
 * > void *__free__(LP, lp_obj self)
 * > {
 * >     free(self.data.val);
 * > }
 * >
 * > lp_obj my_obj = lp_data(LP, 0, my_ptr);
 * > my_obj.data.info->free = __free__;
 */
lp_obj lp_data_new(LP,int magic,void *v) {
	lp_data* data = _lp_data_malloc(lp);
    data->free_fun = 0;
    data->val = v;
    data->magic = magic;
    return ((unsigned int)data) | LP_DATA;
}

/* Function: lp_params
 * Initialize the tinypy parameters.
 *
 * When you are calling a tinypy function, you can use this to initialize the
 * list of parameters getting passed to it. Usually, you may want to use
 * <lp_params_n> or <lp_params_v>.
 */
lp_obj lp_params(LP) {
    lp_obj r;
	lp_list* list;
    r = lp_obj_to_list(lp, lp->_params)->items[lp->cur];
	list = lp_obj_to_list(lp, r);
	for (int i = 0; i < list->len; i++)
	{
		lp_obj_dec(lp, list->items[i]);
	}
    list->len = 0;
    return r;
}

/* Function: lp_params_n
 * Specify a list of objects as function call parameters.
 *
 * See also: <lp_params>, <lp_params_v>
 *
 * Parameters:
 * n - The number of parameters.
 * argv - A list of n tinypy objects, which will be passed as parameters.
 *
 * Returns:
 * The parameters list. You may modify it before performing the function call.
 */
void lp_params_n(LP,int n, lp_obj argv[]) {
    lp_obj* r = lp_params(lp);
    int i; for (i=0; i<n; i++) { _lp_list_append(lp,lp_obj_to_list(lp, r),argv[i]); }
}

/* Function: lp_params_v
 * Pass parameters for a tinypy function call.
 * 
 * When you want to call a tinypy method, then you use this to pass parameters
 * to it.
 * 
 * Parameters:
 * n   - The number of variable arguments following.
 * ... - Pass n tinypy objects, which a subsequently called tinypy method will
 *       receive as parameters.
 * 
 * Returns:
 * A tinypy list object representing the current call parameters. You can modify
 * the list before doing the function call.
 */
void lp_params_v(LP,int n,...) {
    int i;
    lp_obj r = lp_params(lp);
    va_list a; va_start(a,n);
    for (i=0; i<n; i++) {
        _lp_list_append(lp, lp_obj_to_list(lp, r),va_arg(a,lp_obj*));
    }
    va_end(a);
}

void lp_params_v_x(LP, int n, ...) {
	int i;
	lp_obj r = lp_params(lp);
	va_list a; va_start(a, n);
	for (i = 0; i<n; i++) {
		_lp_list_appendx(lp, lp_obj_to_list(lp, r), va_arg(a, lp_obj*));
	}
	va_end(a);
}