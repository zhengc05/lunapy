/*
 * regular expression module
 *
 * Important Note: do not support group name index
 *
 * $Id$
 */

#include <stdio.h>
#include <assert.h>
#include "regexpr.h"
#include "lp.h"

/* last error message */
static const char * LastError = NULL;

/* lower level regex object */
typedef struct {
	struct re_pattern_buffer re_patbuf;	/* The compiled expression */
	struct re_registers re_regs; 		/* The registers from the last match */
	char re_fastmap[256];				/* Storage for fastmap */
	unsigned char *re_translate;		/* String object for translate table */
	unsigned char *re_lastok;			/* String object last matched/searched */

	/* supplementary */
	int re_errno;						/* error num */
	int re_syntax;						/* syntax */
} regexobject;

/* local declarations */
static regexobject* getre(LP, lp_obj* rmobj);
static lp_obj* match_obj_group(LP);
static lp_obj* match_obj_groups(LP);
static lp_obj* match_obj_start(LP);
static lp_obj* match_obj_end(LP);
static lp_obj* match_obj_span(LP);

/*
 * helper function: return lower level regex object
 * rmobj	- regex or match object
 */
static regexobject * getre(LP, lp_obj* rmobj)
{
	lp_obj* reobj_data = lp_getk(lp, rmobj, lp_string(lp, "__data__"));
	regexobject *re = NULL;

	/* validate magic */
	if (reobj_data->data.magic != sizeof(regexobject)) {
		LastError = "broken regex object";
		return (NULL);
	}
	re = (regexobject*)reobj_data->data.val;
	assert(re);

	return (re);
}

/*
 * derive match object from regex object
 */
static lp_obj* match_object(LP, lp_obj* reobj)
{
	lp_obj* mo = lp_object(lp);	/* match object */
	lp_obj* redata;				/* regex object data */
	lp_obj* madata;				/* match object data */
	regexobject *re = NULL;		/* lower level regex object */

	redata = lp_getk(lp, reobj, lp_string(lp, "__data__"));
	re = (regexobject *)redata->data.val;
	assert(re);
	madata = lp_data(lp, (int)sizeof(regexobject), re);

	lp_setkv(lp, mo, lp_string(lp, "group"),	lp_method(lp, mo, match_obj_group));
	lp_setkv(lp, mo, lp_string(lp, "groups"),	lp_method(lp, mo, match_obj_groups));
	lp_setkv(lp, mo, lp_string(lp, "start"),	lp_method(lp, mo, match_obj_start));
	lp_setkv(lp, mo, lp_string(lp, "end"),	lp_method(lp, mo, match_obj_end));
	lp_setkv(lp, mo, lp_string(lp, "span"),	lp_method(lp, mo, match_obj_span));
	lp_setk(lp, mo, lp_string(lp, "__data__"), madata);

	RETURN_LP_OBJ(mo);
}

/*
 * FUNC: regexobj.search(str[,pos=0])
 * self		- regex object
 * str		- string to be searched
 * pos		- optional starting offset
 *
 * RETURN:
 * match object	- when matched
 * None			- not matched
 */
static lp_obj* regex_obj_search(LP)
{
	lp_obj* self = LP_OBJ(0);		/* regex object */
	lp_obj* str = LP_STR(1);
	int pos = LP_INTEGER_DEFAULT(2, 0);
	lp_obj* maobj;				/* match object */
	regexobject *re = NULL;
	int r = -2;					/* -2 indicate exception */
	int range;

	if (pos < 0 || pos > str->string.len) {
		LastError = "search offset out of range";
		goto exception;
	}
	range = str->string.len - pos;

	re = getre(lp, self);
	re->re_lastok = NULL;
	r = re_search(&re->re_patbuf, (unsigned char *)str->string.val,
			str->string.len, pos, range, &re->re_regs);

	/* cannot match pattern */
	if (r == -1)
		goto notfind;

	/* error occurred */
	if (r == -2)
		goto exception;

	/* matched */
	re->re_lastok = (unsigned char *)str->string.val;

	/* match obj */
	maobj = match_object(lp, self);

	return (maobj);	

notfind:
	re->re_lastok = NULL;
	RETURN_LP_NONE;
exception:
	re->re_lastok = NULL;
	lp_raise(0, lp_string(lp, "regex search error"));
}

/*
 * FUNC: regexobj.match(str[,pos=0])
 * self		- regex object
 * str		- string to be matched
 * pos		- optional starting position
 *
 * RETURN:
 * match object	- when matched
 * None			- not matched
 */
static lp_obj* regex_obj_match(LP)
{
	lp_obj* self = LP_OBJ(0);		/* regex object */
	lp_obj* str = LP_STR(1);
	int pos = LP_INTEGER_DEFAULT(2, 0);
	lp_obj* maobj;				/* match object */
	regexobject *re = NULL;
	int r = -2;					/* -2 indicate exception */

	re = getre(lp, self);
	re->re_lastok = NULL;
	r = re_match(&re->re_patbuf, (unsigned char *)str->string.val, 
			str->string.len, pos, &re->re_regs);

	/* cannot match pattern */
	if (r == -1)
		goto nomatch;

	/* error occurred */
	if (r == -2)
		goto exception;

	/* matched */
	re->re_lastok = (unsigned char *)str->string.val;

	/* match obj */
	maobj = match_object(lp, self);

	return (maobj);	

nomatch:
	re->re_lastok = NULL;
	RETURN_LP_NONE;
exception:
	re->re_lastok = NULL;
	lp_raise(0, lp_string(lp, "regex match error"));
}

/*
 * regex object split()
 * self		- regex object
 * restr	- regex string
 * maxsplit	- max split field, default 0, mean no limit
 */
static lp_obj* regex_obj_split(LP)
{
	lp_obj self		= LP_OBJ(0);	/* regex object */
	lp_obj restr	= LP_OBJ(1);	/* string */
	int maxsplit = LP_INTEGER_DEFAULT(2, 0);
	lp_obj maobj;				/* match object */
	regexobject *re = NULL;		/* lower level regex object */
	lp_obj result	= lp_list_new(lp);
	lp_obj grpstr;				/* group string */
	int	slen;					/* string length */
	int srchloc;				/* search location */

	/* maxsplit == 0 means no limit */
	if (maxsplit == 0)
		maxsplit = RE_NREGS;
	assert(maxsplit > 0);

	srchloc = 0;
	slen = strlen((char *)restr->string.val);

	do {
		/* generate a temp match object */
		lp_params_v(lp, 3, self, restr, lp_number_from_int(lp, srchloc));
		maobj = regex_obj_search(lp);
		if (!lp_bool(lp, maobj)) 
			break;

		re = getre(lp, maobj);
		if (re->re_lastok == NULL) {
			lp_raise(0, lp_string(lp, "no match for split()"));
		}

		/* extract fields */
		if (maxsplit > 0) {
			int start = re->re_regs.start[0];
			int end   = re->re_regs.end[0];
			/*printf("%s:start(%d),end(%d)\n", __func__, start, end);*/
			if (start < 0 || end < 0)
				break;

			grpstr = lp_string_copy(lp, 
					(const char *)re->re_lastok + srchloc, start - srchloc);

			if (lp_bool(lp, grpstr)) {
				lp_set(lp, result, lp->lp_None, grpstr);
				maxsplit--;
			}

			srchloc = end;
		}
	} while (srchloc < slen && (int)maxsplit > 0);

	/* collect remaining string, if necessary */
	if (srchloc < slen) {
		grpstr = lp_string_copy(lp, 
				(const char *)restr->string.val + srchloc, slen - srchloc);
		if (lp_bool(lp, grpstr))
			lp_set(lp, result, lp->lp_None, grpstr);
	}

	return (result);
}

/*
 * regex object findall()
 * self		- regex object
 * restr	- regex string
 * pos		- starting position, default 0
 */
static lp_obj* regex_obj_findall(LP)
{
	lp_obj self		= LP_OBJ(0);	/* regex object */
	lp_obj restr	= LP_OBJ(1);	/* string */
	int pos		= LP_INTEGER_DEFAULT(2, 0);
	lp_obj maobj;				/* match object */
	regexobject *re = NULL;		/* lower level regex object */
	lp_obj result	= lp_list_new(lp);
	lp_obj grpstr;				/* group string */
	int	slen;					/* string length */
	int srchloc;				/* search location */

	srchloc = pos;
	slen	= strlen((char *)restr->string.val);
	if (srchloc < 0 || srchloc >= slen)
		lp_raise(0, lp_string(lp, "starting position out of range"));

	do {
		/* generate a temp match object */
		lp_params_v(lp, 3, self, restr, lp_number_from_int(lp, srchloc));
		maobj = regex_obj_search(lp);
		if (!lp_bool(lp, maobj)) 
			break;

		re = getre(lp, maobj);
		if (re->re_lastok == NULL) {
			lp_raise(0, lp_string(lp, "no match for findall()"));
		}

		/* extract fields */
		if (srchloc < slen) {
			int start = re->re_regs.start[0];
			int end   = re->re_regs.end[0];
			/*printf("%s:start(%d),end(%d)\n", __func__, start, end);*/
			if (start < 0 || end < 0)
				break;

			grpstr = lp_string_copy(lp, 
					(const char *)re->re_lastok + start, end - start);

			if (lp_bool(lp, grpstr)) {
				lp_set(lp, result, lp->lp_None, grpstr);
			}

			srchloc = end;
		}
	} while (srchloc < slen);

	return (result);
}

/*
 * FUNC: matchobj.group([group1, ...])
 * self		- match object
 * args		- optional group indices, default 0
 *
 * return specified group.
 */
static lp_obj* match_obj_group(LP)
{
	lp_obj* self = LP_OBJ(0);		/* match object */
	lp_obj* grpidx;				/* a group index */
	regexobject *re = NULL;
	int indices[RE_NREGS];
	int start;
	int end;
	int i;
	int single = 0;				/* single group index? */
	lp_obj* result;

	/* get lower level regex object representation */
	re = getre(lp, self);
	if (re->re_lastok == NULL)
		lp_raise(0, 
				lp_string_new(lp, "group() only valid after successful match/search"));

	for (i = 0; i < RE_NREGS; i++)
		indices[i] = -1;

	/*
	 * if no group index provided, supply default group index 0; else
	 * fill in indices[] with provided group index list.
	 */
	if (lp->params->list->len == 1) {
		indices[0] = 0;
		single = 1;
	} else if (lp->params->list->len == 2) {
		indices[0] = (int)LP_NUM(1);
		single = 1;
	} else {
		i = 0;
		LP_LOOP(1, grpidx)
		if (grpidx->integer < 0 || grpidx->integer > RE_NREGS)
			lp_raise(0, lp_string_new(lp, "group() grpidx out of range"));
		indices[i++] = grpidx->integer;
		LP_END
	}

	/* generate result string list */
	result = lp_list_new(lp);
	for (i = 0; i < RE_NREGS && indices[i] >= 0; i++) {
		lp_obj* grpstr;
		start = re->re_regs.start[indices[i]];
		end   = re->re_regs.end[indices[i]];
		if (start < 0 || end < 0) {
			grpstr = lp_none(lp);
		} else {
			grpstr = lp_string_copy(lp, (const char *)re->re_lastok + start, 
					end - start);
		}
		lp_setv(lp, result, LP_NONE, grpstr);
	}
	return (single ? lp_getk(lp, result, lp_number_from_int(lp, 0)) : result);
}

/*
 * FUNC: matchobj.groups()
 * self	- match object.
 * return all groups.
 * Note: CPython allow a 'default' argument, but we disallow it.
 */
static lp_obj* match_obj_groups(LP)
{
	lp_obj self = LP_OBJ(0);		/* match object */
	regexobject *re = NULL;
	int start;
	int end;
	int i;
	lp_obj result = lp_list_new(lp);

	re = getre(lp, self);
	if (re->re_lastok == NULL) {
		lp_raise(0, 
				lp_string_new(lp, "groups() only valid after successful match/search"));
	}

	for (i = 1; i < RE_NREGS; i++) {
		start = re->re_regs.start[i];
		end   = re->re_regs.end[i];
		if (start < 0 || end < 0)
			break;

		lp_obj* grpstr = lp_string_copy(lp, 
				(const char *)re->re_lastok + start, end - start);

		if (lp_bool(lp, grpstr))
			lp_setv(lp, result, lp->lp_None, grpstr);
	}

	return (result);
}

/*
 * FUNC: matchobj.start([group])
 * self		- match object
 * group	- group index
 * return starting position of matched 'group' substring.
 */
static lp_obj* match_obj_start(LP)
{
	lp_obj* self = LP_OBJ(0);						/* match object */
	int group = LP_INTEGER_DEFAULT(1, 0);	/* group */
	regexobject *re = NULL;
	int start;

	re = getre(lp, self);
	if (re->re_lastok == NULL) {
		lp_raise(0, 
				lp_string(lp, "start() only valid after successful match/search"));
	}

	if (group < 0 || group > RE_NREGS)
		lp_raise(0, lp_string(lp, "IndexError: group index out of range"));

	start = re->re_regs.start[group];

	return (lp_number_from_int(lp, start));
}

/*
 * FUNC: matchobj.end([group])
 * self		- match object
 * group	- group index
 * return ending position of matched 'group' substring.
 */
static lp_obj* match_obj_end(LP)
{
	lp_obj* self = LP_OBJ(0);						/* match object */
	int group = LP_INTEGER_DEFAULT(1, 0);	/* group */
	regexobject *re = NULL;
	int end;

	re = getre(lp, self);
	if (re->re_lastok == NULL) {
		lp_raise(0, 
				lp_string(lp, "end() only valid after successful match/search"));
	}

	if (group < 0 || group > RE_NREGS)
		lp_raise(0, lp_string(lp, "IndexError: group index out of range"));

	end = re->re_regs.end[group];

	return (lp_number_from_int(lp, end));
}

/*
 * FUNC: matchobj.span([group])
 * self		- match object
 * group	- group index
 * return [start,end] position pair of matched 'group' substring.
 */
static lp_obj match_obj_span(LP)
{
	lp_obj self = LP_OBJ(0);						/* match object */
	int group = LP_INTEGER_DEFAULT(1, 0);	/* group */
	regexobject *re = NULL;
	int start;
	int end;
	lp_obj result;

	re = getre(lp, self);
	if (re->re_lastok == NULL) {
		lp_raise(0, 
				lp_string_new(lp, "span() only valid after successful match/search"));
	}

	if (group < 0 || group > RE_NREGS)
		lp_raise(0, lp_string_new(lp, "IndexError: group index out of range"));

	start = re->re_regs.start[group];
	end   = re->re_regs.end[group];

	result = lp_list_new(lp);
	lp_setv(lp, result, LP_NONE, lp_number_from_int(lp, start));
	lp_setv(lp, result, LP_NONE, lp_number_from_int(lp, end));

	return (result);
}

/*
 * compile out a re object
 * repat	- regex pattern
 * resyn	- regex syntax
 */
static lp_obj* regex_compile(LP)
{
	char *error = NULL;
	char const *pat = NULL;
	int size = 0;
	lp_obj* reobj_data;
	lp_obj* repat = LP_TYPE(0, LP_STRING);						/* pattern */
	int resyn = LP_INTEGER_DEFAULT(1, RE_SYNTAX_EMACS);	/* syntax */
	lp_obj* reobj;	/* regex object */
	regexobject *re;

	/*
	 * create regex object, its parent is builtin 'object'
	 */
	reobj = lp_object(lp);

	re = (regexobject *)malloc(sizeof(regexobject));
	if (!re) {
		error = "malloc lower level regex object failed";
		goto finally;
	}

	re->re_patbuf.buffer = NULL;
	re->re_patbuf.allocated = 0;
	re->re_patbuf.fastmap = (unsigned char *)re->re_fastmap;
	re->re_patbuf.translate = NULL;
	re->re_translate = NULL;
	re->re_lastok = NULL;

	re->re_errno = 0;
	re->re_syntax = resyn;

	pat = repat->string.val;
	size = repat->string.len;
	error = re_compile_pattern((unsigned char *)pat, size, &re->re_patbuf);
	if (error != NULL) {
		LastError = error;
		goto finally;
	}

	/* regexobject's size as magic */
	reobj_data = lp_data_new(lp, (int)sizeof(regexobject), re);

	/*
	 * bind to regex object
	 */
	lp_setkv(lp, reobj, lp_string_new(lp, "search"),
			lp_method(lp, reobj, regex_obj_search));
	lp_setkv(lp, reobj, lp_string_new(lp, "match"),
			lp_method(lp, reobj, regex_obj_match));
	lp_setkv(lp, reobj, lp_string_new(lp, "split"),
			lp_method(lp, reobj, regex_obj_split));
	lp_setkv(lp, reobj, lp_string_new(lp, "findall"),
			lp_method(lp, reobj, regex_obj_findall));
	lp_setkv(lp, reobj, lp_string_new(lp, "__data__"), reobj_data);

	lp_setkv(lp, reobj, lp_string_new(lp, "__name__"),
			lp_string_new(lp, "regular expression object"));
	lp_setkv(lp, reobj, lp_string_new(lp, "__doc__"), lp_string_new(lp,
				"regular expression object, support methods:\n"
				"search(str[,pos=0])-search 'str' from 'pos'\n"
				"match(str[,pos=0])	-match 'str' from 'pos'\n"
				));

	return (reobj);

finally:
	lp_raise(0, lp_string_new(lp, error));
}

/*
 * module level search()
 */
static lp_obj* regex_search(LP)
{
	lp_obj* repat = LP_OBJ(0);	/* pattern */
	lp_obj* restr = LP_OBJ(1);	/* string */
	int resyn = LP_INTEGER_DEFAULT(2, RE_SYNTAX_EMACS);
	lp_obj* reobj;				/* regex object */
	lp_obj* maobj;				/* match object */

	/* compile out regex object */
	LP_OBJ_INC(repat);
	lp_params_v_x(lp, 2, repat, lp_number_from_int(lp, resyn));
	reobj = regex_compile(lp);
	LP_OBJ_INC(restr);
	if (!reobj)
		return 0;
	/* call r.search() */
	lp_params_v_x(lp, 3, reobj, restr, lp_number_from_int(lp, 0));
	maobj = regex_obj_search(lp);

	return (maobj);
}

/*
 * module level match()
 */
static lp_obj* regex_match(LP)
{
	lp_obj* repat = LP_OBJ(0);	/* pattern */
	lp_obj* restr = LP_OBJ(1);	/* string */
	int resyn = LP_INTEGER_DEFAULT(2, RE_SYNTAX_EMACS);
	lp_obj* reobj;				/* regex object */
	lp_obj* maobj;				/* match object */

	/* compile out regex object */
	LP_OBJ_INC(repat);
	lp_params_v_x(lp, 2, repat, lp_number_from_int(lp, resyn));
	reobj = regex_compile(lp);
	LP_OBJ_INC(restr);
	if (!reobj)
		return 0;
	/* call r.search() */
	lp_params_v_x(lp, 3, reobj, restr, lp_number_from_int(lp, 0));
	maobj = regex_obj_match(lp);

	return (maobj);
}

/*
 * module level split()
 * repat	- regex pattern
 * restr	- regex string
 * maxsplit	- max split field, default 0, mean no limit
 */
static lp_obj* regex_split(LP)
{
	lp_obj* repat = LP_OBJ(0);	/* pattern */
	lp_obj* restr = LP_OBJ(1);	/* string */
	int maxsplit = LP_INTEGER_DEFAULT(2, 0);
	lp_obj* reobj;				/* regex object */

	/* generate a temp regex object */
	LP_OBJ_INC(repat);
	lp_params_v_x(lp, 2, repat, lp_number_from_int(lp, RE_SYNTAX_EMACS));
	reobj = regex_compile(lp);
	
	LP_OBJ_INC(restr);
	lp_params_v_x(lp, 3, reobj, restr, lp_number_from_int(lp, maxsplit));
	return regex_obj_split(lp);
}

/*
 * module level findall()
 * repat	- regex pattern
 * restr	- regex string
 * resyn	- regex syntax, optional, default RE_SYNTAX_EMAC
 */
static lp_obj* regex_findall(LP)
{
	lp_obj* repat = LP_OBJ(0);	/* pattern */
	lp_obj* restr = LP_OBJ(1);	/* string */
	int resyn = LP_INTEGER_DEFAULT(2, RE_SYNTAX_EMACS);
	lp_obj* reobj;				/* regex object */

	/* generate a temp regex object */
	LP_OBJ_INC(repat);
	lp_params_v_x(lp, 2, repat, lp_number_from_int(lp, resyn));
	reobj = regex_compile(lp);
	
	LP_OBJ_INC(restr);
	lp_params_v_x(lp, 2, reobj, restr);
	return regex_obj_findall(lp);
}


/*
 * re mod can only support 'set_syntax', 'get_syntax', and 'compile' functions,
 * 'compile' function will return a 'reobj', and this 'reobj' will support
 * methods 'search', 'match', 'group', 'groupall', el al.
 */
void re_init(LP)
{
	/*
	 * module dict for re
	 */
	lp_obj re_mod = lp_dict_new(lp);

	/*
	 * bind to re module
	 */
	lp_setkv(lp, re_mod, lp_string_new(lp, "compile"),	  lp_fnc_n(lp, regex_compile));
	lp_setkv(lp, re_mod, lp_string_new(lp, "search"),		  lp_fnc_n(lp, regex_search));
	lp_setkv(lp, re_mod, lp_string_new(lp, "match"),		  lp_fnc_n(lp, regex_match));
	lp_setkv(lp, re_mod, lp_string_new(lp, "split"),		  lp_fnc_n(lp, regex_split));
	lp_setkv(lp, re_mod, lp_string_new(lp, "findall"),	  lp_fnc_n(lp, regex_findall));
	lp_setkv(lp, re_mod, lp_string_new(lp, "AWK_SYNTAX"),   lp_number_from_int(lp, RE_SYNTAX_AWK));
	lp_setkv(lp, re_mod, lp_string_new(lp, "EGREP_SYNTAX"), lp_number_from_int(lp, RE_SYNTAX_EGREP));
	lp_setkv(lp, re_mod, lp_string_new(lp, "GREP_SYNTAX"),  lp_number_from_int(lp, RE_SYNTAX_GREP));
	lp_setkv(lp, re_mod, lp_string_new(lp, "EMACS_SYNTAX"), lp_number_from_int(lp, RE_SYNTAX_EMACS));

	/*
	 * bind special attibutes to re module
	 */
	lp_setkv(lp, re_mod, lp_string_new(lp, "__name__"), 
			lp_string_new(lp, "regular expression module"));
	lp_setkv(lp, re_mod, lp_string_new(lp, "__file__"), lp_string_new(lp, __FILE__));
	lp_setkv(lp, re_mod, lp_string_new(lp, "__doc__"),
			lp_string_new(lp, "simple regular express implementation"));

	/*
	 * bind regex module to tinypy modules[]
	 */
	lp_setkv(lp, lp->modules, lp_string_new(lp, "re"), re_mod);
}

