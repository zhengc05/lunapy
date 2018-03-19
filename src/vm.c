#include <stdio.h>

#include "lp.h"
#include "lp_internal.h"
#include "tokenize.h"

extern void math_init(LP);
extern void random_init(LP);
extern void re_init(LP);
extern void time_init(LP);
extern void init_lp_mem(LP);
int lp_run(LP, int cur);

/* File: VM
 * Functionality pertaining to the virtual machine.
 */

lp_vm *_lp_init(void) {
    int i;
    lp_vm *lp = (lp_vm*)calloc(sizeof(lp_vm),1);
	init_lp_mem(lp);
    lp->time_limit = LP_NO_LIMIT;
    lp->clocks = clock();
    lp->time_elapsed = 0.0;
    lp->mem_limit = LP_NO_LIMIT;
    lp->mem_exceeded = 0;
    lp->mem_used = sizeof(lp_vm);
	memset(lp->frames, 0, sizeof(lp->frames));
    lp->cur = 0;
    lp->ex = 0;
	lp->oldex = 0;
    lp->root = lp_list_nt(lp);
    for (i=0; i<256; i++) { lp->chars[i][0]=i; }
    lp->_regs = lp_list(lp);
    for (i=0; i<LP_REGS; i++) { lp_set(lp,lp->_regs,lp->lp_None, lp->lp_None); }
    lp->builtins = lp_dict(lp);
	lp->path = lp_list(lp);
    lp->modules = lp_dict(lp);
    lp->_params = lp_list(lp);
    for (i=0; i<LP_FRAMES; i++) { lp_setv(lp,lp->_params, lp->lp_None,lp_list(lp)); }
    lp_set(lp,lp->root, lp->lp_None,lp->builtins);
    lp_set(lp,lp->root, lp->lp_None,lp->modules);
    lp_set(lp,lp->root, lp->lp_None,lp->_regs);
    lp_set(lp,lp->root, lp->lp_None,lp->_params);
    lp_setk(lp,lp->builtins,lp_string(lp, "MODULES"),lp->modules);
    lp_setk(lp,lp->modules,lp_string(lp, "BUILTINS"),lp->builtins);
    lp_setk(lp,lp->builtins,lp_string(lp, "BUILTINS"),lp->builtins);
    lp_obj* sys = lp_dict(lp);
    lp_setkv(lp, sys, lp_string(lp, "version"), lp_string(lp, "tinypy 1.2+SVN"));
    lp_setk(lp,lp->modules, lp_string(lp, "sys"), sys);
	lp_setk(lp, sys, lp_string(lp, "path"), lp->path);
	_lp_list_appendx(lp, lp->path->list, lp_string(lp, "."));
    lp->regs = lp->_regs->list->items;
    return lp;
}


/* Function: lp_deinit
 * Destroys a VM instance.
 * 
 * When you no longer need an instance of tinypy, you can use this to free all
 * memory used by it. Even when you are using only a single tinypy instance, it
 * may be good practice to call this function on shutdown.
 */
void lp_deinit(LP) {
    while (lp->root->list->len) {
        _lp_list_pop(lp,lp->root->list,0,"lp_deinit");
    }
    lp->mem_used -= sizeof(lp_vm); 
    free(lp);
}

/* lp_frame_*/
void lp_frame(LP,lp_obj* globals,lp_obj* code,lp_obj **ret_dest) {
    lp_frame_ *f = &lp->frames[lp->cur+1];
	lp_obj** regs = (lp->cur <= 0 ? lp->regs : lp->frames[lp->cur].regs + lp->frames[lp->cur].cregs);

	if (regs + (256 + LP_REGS_EXTRA) >= lp->regs + LP_REGS || lp->cur >= LP_FRAMES - 1) {
		lp_raise(, lp_string(lp, "(lp_frame) RuntimeError: stack overflow"));
	}

	LP_OBJ_DEC(f->globals);
    f->globals = globals;
	LP_OBJ_INC(globals);
	LP_OBJ_DEC(f->code);
    f->code = code;
	LP_OBJ_INC(code);
    f->cur = (lp_code*)f->code->string.val;
    f->jmp = 0;
/*     fprintf(stderr,"lp->cur: %d\n",lp->cur);*/
	f->regs = regs;
    
	LP_OBJ_DEC(f->regs[0]);
    f->regs[0] = f->globals;
	LP_OBJ_INC(globals);
	LP_OBJ_DEC(f->regs[1]);
    f->regs[1] = f->code;
	LP_OBJ_INC(code);
    f->regs += LP_REGS_EXTRA;
    
    f->ret_dest = ret_dest;
    f->lineno = 0;
	LP_OBJ_DEC(f->line);
    f->line = lp_string(lp, "");
	LP_OBJ_DEC(f->name);
    f->name = lp_string(lp, "?");
	LP_OBJ_DEC(f->fname);
    f->fname = lp_string(lp, "?");
    f->cregs = 0;
/*     return f;*/
    
    lp->cur += 1;

}

void lp_print_stack(LP) {
    int i;
    printf("\n");
    for (i=0; i<=lp->cur; i++) {
        if (!lp->frames[i].lineno) { continue; }
        printf("File \""); lp_echo(lp,lp->frames[i].fname); printf("\", ");
        printf("line %d, in ",lp->frames[i].lineno);
        lp_echo(lp,lp->frames[i].name); printf("\n ");
        lp_echo(lp,lp->frames[i].line); printf("\n");
    }
    printf("\nException:\n"); lp_echo(lp,lp->ex); printf("\n");
}

int lp_handle(LP) {
    int i;
    for (i=lp->cur; i>=0; i--) {
        if (lp->frames[i].jmp) { break; }
    }
    if (i >= 0) {
		LP_OBJ_DEC(lp->oldex);
		lp->oldex = lp->ex;
		lp->ex = 0;
        lp->cur = i;
        lp->frames[i].cur = lp->frames[i].jmp;
        lp->frames[i].jmp = 0;
        return 1;
    }

    return 0;
}

/* Function: lp_call
 * Calls a tinypy function.
 *
 * Use this to call a tinypy function.
 *
 * Parameters:
 * lp - The VM instance.
 * self - The object to call.
 * params - Parameters to pass.
 *
 * Example:
 * > lp_call(lp,
 * >     lp_get(lp, lp->builtins, lp_string("foo")),
 * >     lp_params_v(lp, lp_string("hello")))
 * This will look for a global function named "foo", then call it with a single
 * positional parameter containing the string "hello".
 */
lp_obj* lp_call(LP,lp_obj* self) {
    /* I'm not sure we should have to do this, but
    just for giggles we will. */
	lp_obj* params = lp->params;
	lp_obj* r;

    if (self->type == LP_DICT) {
        if (self->dict.dtype == 1) {
            lp_obj *meta;
			if (lp_lookupx(lp,self,"__new__",&meta)) {
                _lp_list_insert(lp,params->list,0,self);
                r = lp_call(lp,meta);
				LP_OBJ_DEC(meta);
				return r;
            }
        } else if (self->dict.dtype == 2) {
            LP_META_BEGIN(self,"__call__");
                r = lp_call(lp,meta);
				LP_OBJ_DEC(meta);
				return r;
            LP_META_END;
        }
    }
    if (self->type == LP_FNC && !(self->fnc.ftype&1)) {
        lp_obj* r = lp_tcall(lp,self);
        return r;
    }
    if (self->type == LP_FNC) {
        lp_obj* dest = lp->lp_None;
        lp_frame(lp,self->fnc.info->globals,self->fnc.info->code,&dest);
        if ((self->fnc.ftype&2)) {
            lp->frames[lp->cur].regs[0] = params; LP_OBJ_INC(params);
            _lp_list_insert(lp,params->list,0,self->fnc.info->self);
        } else {
            lp->frames[lp->cur].regs[0] = params; LP_OBJ_INC(params);
        }
		if (!lp_run(lp, lp->cur)) return 0;
		if (!dest) return 0;
        RETURN_LP_OBJ(dest);
    }
    lp_params_v(lp,1,self); lpf_print(lp);
    lp_raise(0,lp_string(lp, "(lp_call) TypeError: object is not callable"));
}


void lp_return(LP, lp_obj* v) {
    lp_obj **dest = lp->frames[lp->cur].ret_dest;
    if (dest) { *dest = v; LP_OBJ_INC(v); }
	for (int i = -LP_REGS_EXTRA; i < lp->frames[lp->cur].cregs; i++)
	{
		LP_OBJ_DEC(lp->frames[lp->cur].regs[i]);
	}
    memset(lp->frames[lp->cur].regs-LP_REGS_EXTRA,0,(LP_REGS_EXTRA+lp->frames[lp->cur].cregs)*sizeof(lp_obj*));
    lp->cur -= 1;
}

enum {
    LP_IEOF,LP_IADD,LP_ISUB,LP_IMUL,LP_IDIV,LP_IPOW,LP_IBITAND,LP_IBITOR,LP_ICMP,LP_IGET,LP_ISET,
    LP_INUMBER,LP_ISTRING,LP_IGGET,LP_IGSET,LP_IMOVE,LP_IDEF,LP_IPASS,LP_IJUMP,LP_ICALL,
    LP_IRETURN,LP_IIF,LP_IDEBUG,LP_IEQ,LP_ILE,LP_ILT,LP_IDICT,LP_ILIST,LP_INONE,LP_ILEN,
    LP_ILINE,LP_IPARAMS,LP_IIGET,LP_IFILE,LP_INAME,LP_INE,LP_IHAS,LP_IRAISE,LP_ISETJMP,
    LP_IMOD,LP_ILSH,LP_IRSH,LP_IITER,LP_IDEL,LP_IREGS,LP_IBITXOR, LP_IIFN, 
    LP_INOT, LP_IBITNOT,
    LP_ITOTAL
};

/* char *lp_strings[LP_ITOTAL] = {
       "EOF","ADD","SUB","MUL","DIV","POW","BITAND","BITOR","CMP","GET","SET","NUM",
       "STR","GGET","GSET","MOVE","DEF","PASS","JUMP","CALL","RETURN","IF","DEBUG",
       "EQ","LE","LT","DICT","LIST","NONE","LEN","LINE","PARAMS","IGET","FILE",
       "NAME","NE","HAS","RAISE","SETJMP","MOD","LSH","RSH","ITER","DEL","REGS",
       "BITXOR", "IFN", "NOT", "BITNOT",
   };*/

#define VA ((int)e->regs.a)
#define VB ((int)e->regs.b)
#define VC ((int)e->regs.c)
#define RA regs[e->regs.a]
#define RB regs[e->regs.b]
#define RC regs[e->regs.c]
#define UVBC (unsigned short)(((VB<<8)+VC))
#define SVBC (short)(((VB<<8)+VC))
#define SR(v) f->cur = cur; return(v);


int lp_step(LP) {
    lp_frame_ *f = &lp->frames[lp->cur];
    lp_obj **regs = f->regs;
    lp_code *cur = f->cur;
	lp_obj *r;
    while(1) {

    lp_code *e = cur;
	if (lp->ex) { SR(1); }
    switch (e->i) {
		case LP_IEOF: lp_return(lp, lp->lp_None); SR(0); break;
		case LP_IADD: r = lp_add(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ISUB: r = lp_sub(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IMUL: r = lp_mul(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IDIV: r = lp_div(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IPOW: r = lp_pow(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IBITAND: r = lp_bitwise_and(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IBITOR:  r = lp_bitwise_or(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IBITXOR:  r = lp_bitwise_xor(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IMOD:  r = lp_mod(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ILSH:  r = lp_lsh(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IRSH:  r = lp_rsh(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ICMP: r = lp_number_from_int(lp, lp_cmp(lp, RB, RC)); LP_OBJ_DEC(RA); RA = r; break;
		case LP_INE: r = lp_number_from_int(lp, lp_cmp(lp, RB, RC) != 0); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IEQ: r = lp_number_from_int(lp, lp_cmp(lp, RB, RC) == 0); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ILE: r = lp_number_from_int(lp, lp_cmp(lp, RB, RC) <= 0); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ILT: r = lp_number_from_int(lp, lp_cmp(lp, RB, RC) < 0); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IBITNOT:  r = lp_bitwise_not(lp, RB); LP_OBJ_DEC(RA); RA = r; break;
		case LP_INOT: r = lp_number_from_int(lp, !lp_bool(lp, RB)); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IPASS: break;
        case LP_IIF: if (lp_bool(lp,RA)) { cur += 1; } break;
        case LP_IIFN: if (!lp_bool(lp,RA)) { cur += 1; } break;
		case LP_IGET: r = lp_get(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
        case LP_IITER:
            if (lp_type_number(lp, RC) < lp_lenx(lp,RB)) {
				r = lp_iter(lp,RB,RC);
				LP_OBJ_DEC(RA);
				RA = r;
				switch (RC->type)
				{
				case LP_INT:
					RC->integer ++;
					break;
				case LP_DOUBLE:
					RC->doublen += 1;
					break;
				}

                cur += 1;
            }
            break;
		case LP_IHAS: r = lp_has(lp, RB, RC); LP_OBJ_DEC(RA); RA = r; break;
		case LP_IIGET: lp_iget(lp, &RA, RB, RC); break;
        case LP_ISET: lp_set(lp,RA,RB,RC); break;
        case LP_IDEL: lp_del(lp,RA,RB); break;
		case LP_IMOVE:  r = RB; LP_OBJ_INC(r); LP_OBJ_DEC(RA); RA = r; break;
        case LP_INUMBER:

			LP_OBJ_DEC(RA);
			if (VB)
			{
				RA = lp_number_from_double(lp, *(double*)(*++cur).string.val);
				cur += sizeof(double) / 4;
			}
			else
			{
				RA = lp_number_from_int(lp, *(int*)(*++cur).string.val);
				cur += sizeof(int) / 4;
			}
            continue;
        case LP_ISTRING: {

			LP_OBJ_DEC(RA);
            int a = (*(cur+1)).string.val-f->code->string.val;
			int l = UVBC;
            RA = lp_string_sub(lp,f->code,a,a+l),
            cur += (l/4)+1;
            }
            break;
		case LP_IDICT: r = lp_dict_n(lp, VC / 2, &RB); LP_OBJ_DEC(RA); RA = r; break;
		case LP_ILIST: r = lp_list_n(lp, VC, &RB); LP_OBJ_DEC(RA); RA = r; break;
        case LP_IPARAMS: lp_params_n(lp,VC,&RB); break;
		case LP_ILEN: r = lp_len(lp, RB); LP_OBJ_DEC(RA); RA = r; break;
        case LP_IJUMP: cur += SVBC; continue; break;
        case LP_ISETJMP: f->jmp = SVBC?cur+SVBC:0; break;
        case LP_ICALL:

			f->cur = cur + 1;  r = lp_call(lp, RB); LP_OBJ_DEC(RA); RA = r;
            return 0; break;
        case LP_IGGET:
            if (!lp_iget(lp,&r,f->globals,RB)) {
                r = lp_get(lp,lp->builtins,RB);
            }
			LP_OBJ_DEC(RA);
			RA = r;
            break;
        case LP_IGSET: lp_set(lp,f->globals,RA,RB); break;
        case LP_IDEF: {

            int a = (*(cur+1)).string.val-f->code->string.val;
			lp_obj* c = lp_string_sub(lp, f->code, a, a + (SVBC - 1) * 4);
			r = lp_def(lp,c,f->globals);
			LP_OBJ_DEC(RA);
			RA = r;
			LP_OBJ_DEC(c);
            cur += SVBC; continue;
            }
            break;
            
        case LP_IRETURN: lp_return(lp,RA); SR(0); break;
		case LP_IRAISE: LP_OBJ_DEC(lp->ex); lp->ex = RA; LP_OBJ_INC(lp->ex); break;
        case LP_IDEBUG:
			{
				lp_obj* a = lp_string(lp, "DEBUG:");
				lp_obj* b = lp_number_from_int(lp, VA);
				lp_params_v(lp, 3, a, b, RA);
				LP_OBJ_DEC(a);
				LP_OBJ_DEC(b);
				lpf_print(lp);
			}
            break;
		case LP_INONE: LP_OBJ_DEC(RA); RA = lp->lp_None; LP_OBJ_INC(lp->lp_None); break;
        case LP_ILINE:

            ;
            int a = (*(cur+1)).string.val-f->code->string.val;
			LP_OBJ_DEC(f->line);
            f->line = lp_string_sub(lp,f->code,a,a+VA*4-1);
/*             fprintf(stderr,"%7d: %s\n",UVBC,f->line.string.val);*/
            cur += VA; f->lineno = UVBC;
            break;
		case LP_IFILE: LP_OBJ_DEC(f->fname); f->fname = RA; LP_OBJ_INC(RA); break;
		case LP_INAME: LP_OBJ_DEC(f->name); f->name = RA; LP_OBJ_INC(RA); break;
        case LP_IREGS: f->cregs = VA; break;
        default:
            lp_raise(1,lp_string(lp, "(lp_step) RuntimeError: invalid instruction"));
            break;
    }

    cur += 1;
    }
    SR(0);
}

#define debug(fmt, ...) { lp_obj* t = lp_printf(lp, "%d : "fmt, (int)(cur-begin), __VA_ARGS__); _lp_list_appendx(lp, out->list, t); }

lp_obj* lp_disasm(LP, lp_obj* code)
{
	lp_obj* out = lp_list(lp);
	lp_code *cur = (lp_code *)code->string.val;
	lp_code *begin = cur, *end = cur + code->string.len / sizeof(lp_code);
	while (1) {
#ifdef LP_SANDBOX
		lp_bounds(lp, cur, 1);
#endif
		lp_code* e = cur;
		/*
		fprintf(stderr,"%2d.%4d: %-6s %3d %3d %3d\n",lp->cur,cur - (lp_code*)f->code.string.val,lp_strings[e.i],VA,VB,VC);
		int i; for(i=0;i<16;i++) { fprintf(stderr,"%d: %s\n",i,LP_xSTR(regs[i])); }
		*/
		switch (e->i) {
		case LP_IEOF: debug("EOF"); break;
		case LP_IADD: debug("[%d] = [%d] + [%d]", VA, VB, VC); break;
		case LP_ISUB: debug("[%d] = [%d] - [%d]", VA, VB, VC); break;
		case LP_IMUL: debug("[%d] = [%d] * [%d]", VA, VB, VC); break;
		case LP_IDIV: debug("[%d] = [%d] / [%d]", VA, VB, VC); break;
		case LP_IPOW: debug("[%d] = [%d] ** [%d]", VA, VB, VC); break;
		case LP_IBITAND: debug("[%d] = [%d] & [%d]", VA, VB, VC); break;
		case LP_IBITOR: debug("[%d] = [%d] | [%d]", VA, VB, VC); break;
		case LP_IBITXOR: debug("[%d] = [%d] ^ [%d]", VA, VB, VC); break;
		case LP_IMOD: debug("[%d] = [%d] % [%d]", VA, VB, VC); break;
		case LP_ILSH: debug("[%d] = [%d] << [%d]", VA, VB, VC); break;
		case LP_IRSH: debug("[%d] = [%d] >> [%d]", VA, VB, VC); break;
		case LP_ICMP: debug("[%d] = [%d] < [%d]", VA, VB, VC); break;
		case LP_INE: debug("[%d] = [%d] != [%d]", VA, VB, VC); break;
		case LP_IEQ: debug("[%d] = [%d] == [%d]", VA, VB, VC); break;
		case LP_ILE: debug("[%d] = [%d] <= [%d]", VA, VB, VC); break;
		case LP_ILT: debug("[%d] = [%d] < [%d]", VA, VB, VC); break;
		case LP_IBITNOT: debug("[%d] = ![%d]", VA, VB); break;
		case LP_INOT: debug("[%d] = not [%d]", VA, VB); break;
		case LP_IPASS: debug("pass"); break;
		case LP_IIF: debug("if not [%d]", VA); break;
		case LP_IIFN: debug("if [%d]", VA); break;
		case LP_IGET: debug("[%d] = [%d] get [%d]", VA, VB, VC); break;
		case LP_IITER:
			debug("[%d] = iter [%d] [%d]++", VA, VB, VC, VC);
			break;
		case LP_IHAS: debug("[%d] = [%d] has [%d]", VA, VB, VC); break;
		case LP_IIGET: debug("[%d] = [%d] iget [%d]", VA, VB, VC); break;
		case LP_ISET: debug("[%d].[%d] = [%d]", VA, VB, VC); break;
		case LP_IDEL: debug("[%d] del [%d]", VA, VB); break;
		case LP_IMOVE: debug("[%d] = [%d]", VA, VB); break;
		case LP_INUMBER:
			if (VB)
			{
				debug("[%d] = %f", VA, *(double*)(*++cur).string.val);
				cur += sizeof(double) / 4;
			}
			else
			{
				debug("[%d] = %d", VA, *(int*)(*++cur).string.val);
				cur += sizeof(int) / 4;
			}
			continue;
		case LP_ISTRING:
		{
			/* RA = lp_string_n((*(cur+1)).string.val,UVBC); */
			int a = (*(cur + 1)).string.val - code->string.val;
			int l = UVBC;
			lp_obj* s = lp_string_sub(lp, code, a, a + l);
			debug("[%d] = \"%s\"", VA, s->string.val);
			LP_OBJ_DEC(s);
			cur += (l / 4) + 1;
		}
		break;
		case LP_IDICT: debug("[%d] = dict %d/2 [%d]", VA, VC, VB); break;
		case LP_ILIST: debug("[%d] = list %d [%d]", VA, VC, VB); break;
		case LP_IPARAMS: debug("params %d [%d]", VC, VB); break;
		case LP_ILEN: debug("[%d] = len [%d]", VA, VB); break;
		case LP_IJUMP: debug("jmp %d", SVBC + (int)(cur - begin)); break;
		case LP_ISETJMP: debug("setjmp %d", SVBC + (int)(cur - begin)); break;
		case LP_ICALL:
			debug("[%d] = [%d] (  )", VA, VB);
			break;
		case LP_IGGET:
			debug("[%d] = _G.[%d]", VA, VB);
			break;
		case LP_IGSET: debug("_G.[%d] = [%d]", VA, VB); break;
		case LP_IDEF:
		{
			/*            RA = lp_def(lp,(*(cur+1)).string.val,f->globals);*/
			debug("[%d] = def %d", VA, SVBC + (int)(cur - begin));
		}
		break;

		case LP_IRETURN: debug("return [%d]", VA); break;
		case LP_IRAISE: debug("raise [%d]", VA); break;
		case LP_IDEBUG:
			debug("DEBUG %d [%d]", VA, VA);
			break;
		case LP_INONE: debug("[%d] = None", VA); break;
		case LP_ILINE:
		{
			int a = (*(cur + 1)).string.val - code->string.val;
			/*            f->line = lp_string_n((*(cur+1)).string.val,VA*4-1);*/
			lp_obj* s = lp_string_sub(lp, code, a, a + VA * 4 - 1);
			/*             fprintf(stderr,"%7d: %s\n",UVBC,f->line.string.val);*/
			debug("line = \"%s\"", s->string.val);
			LP_OBJ_DEC(s);
			cur += VA;
		}
			break;
		case LP_IFILE: debug("fname = [%d]", VA); break;
		case LP_INAME: debug("name = [%d]", VA); break;
		case LP_IREGS: debug("cregs = %d", VA); break;
		default:
			lp_raise(0, lp_string(lp, "(lp_step) RuntimeError: invalid instruction"));
			break;
		}
		cur += 1;
		if (cur == end)
			break;
	}
	lp_params_v(lp, 2, lp_string(lp, "\n"), out);
	
	return lpf_join(lp);
}

lp_obj* lpf_disasm(LP)
{
	lp_obj* code = LP_OBJ(0);
	lp_obj* r = lp_disasm(lp, code);
	return r;
}

int lp_run(LP,int cur) {
	while (lp->cur >= cur)
	{
		if (lp_step(lp))
		{
			if (!lp_handle(lp))
				return 0;
		}
	}

	return 1;
}


lp_obj* lp_ez_call(LP, const char *mod, const char *fnc)
{
    lp_obj *tmp, *tmp2, *r;
    tmp = lp_get(lp,lp->modules,lp_string(lp, mod));
    tmp2 = lp_get(lp,tmp,lp_string(lp, fnc));
    r = lp_call(lp,tmp2);
	LP_OBJ_DEC(tmp2);
	LP_OBJ_DEC(tmp);
	return r;
}

lp_obj* lp_import(LP, lp_obj* fname, lp_obj* name, lp_obj* code)
{
	char filename[256], *content = 0;
    lp_obj *g, *file;

    if ((fname->type != LP_NONE && _lp_str_index(fname,0,lp_string(lp, ".py"))!=-1 && code->type == LP_NONE)) {
		int size;
		lp_obj* path = lp->path;
		for (int i = 0; i < path->list->len; i++)
		{
			int j = 0;
			lp_obj* p = path->list->items[i];
			if (p->type == LP_STRING)
			{
				struct stat stbuf;
				if (!strcmp(p->string.val, "."))
				{
					file = lp_getk(lp, lp->frames[lp->cur].globals, lp_string(lp, "__file__"));
					j = file->string.len;
					while (j > 0)
					{
						j--;
						if (file->string.val[j] == '\\' || file->string.val[j] == '/')
						{
							j++;
							break;
						}
					}
					if (j + fname->string.len > 255) continue;
					memcpy(filename, file->string.val, j);
					LP_OBJ_DEC(file);
				}
				else
				{
					j = p->string.len + 1;
					if (j + fname->string.len > 255) continue;
					memcpy(filename, p->string.val, p->string.len);
					filename[p->string.len] = '/';
				}

				memcpy(&filename[j], fname->string.val, fname->string.len);
				filename[j + fname->string.len] = '\0';
				if (stat(filename, &stbuf) == 0)
				{
					int l = stbuf.st_size, result;
					const char* rc;
					FILE* f = fopen(filename, "rb");
					content = (char*)malloc(l + 1);
					fread(content, 1, l, f);
					content[l] = '\0';
					/*    if (rr !=l) { printf("hmmn: %d %d\n",rr,(int)l); }*/
					fclose(f);
					rc = compile(filename, content, &size, &result);
					if (!result)
					{
						lp_obj *e = lp_string_copy(lp, rc, strlen(rc) + 1);
						free(rc);
						lp_raise(0, e);
					}
					code = lp_string_copy(lp, rc, size);
					free(rc);
					break;
				}
			}
			else if (p->type == LP_FNC)
			{
				lp_obj* f;
				lp_params_v(lp, 1, fname);
				file = lp_call(lp, p);
				if (file->type != LP_NONE)
				{
					f = lp_getk(lp, file, lp_number_from_int(lp, 0));
					code = lp_getk(lp, file, lp_number_from_int(lp, 1));
					strncpy(filename, f->string.val, 256);
					LP_OBJ_DEC(f);
					break;
				}
				LP_OBJ_DEC(file);
			}
		}
    }

    if (code->type == LP_NONE) {
		lp_raise(0, lp_string(lp, "lp_import TypeError: ? "));
    }

    g = lp_dict(lp);
	lp_setkv(lp,g,lp_string(lp, "__file__"), lp_string_copy(lp, filename, strlen(filename)+1));
    lp_setk(lp,g,lp_string(lp, "__name__"),name);
    lp_setk(lp,g,lp_string(lp, "__code__"),code);
    lp_setk(lp,g,lp_string(lp, "__dict__"),g);
    lp_frame(lp,g,code,0);
	LP_OBJ_DEC(code);
    lp_set(lp,lp->modules,name,g);

    //if (!lp->jmp)
	{
		lp_run(lp,lp->cur);
	}

    return g;
}


/* Function: lp_import
 * Imports a module.
 * 
 * Parameters:
 * fname - The filename of a file containing the module's code.
 * name - The name of the module.
 * codes - The module's code.  If this is given, fname is ignored.
 * len - The length of the bytecode.
 *
 * Returns:
 * The module object.
 */
lp_obj* lp_import_(LP, const char * fname, const char * name, void *codes, int len)
{
    lp_obj* f = lp_string(lp, fname);
    lp_obj* bc = lp_string_n(lp, (const char*)codes,len);
	lp_obj* n = lp_string(lp, name);
    lp_obj* module = lp_import(lp,f,n,bc);
	LP_OBJ_DEC(f);
	LP_OBJ_DEC(bc);
	LP_OBJ_DEC(n);
	return module;
}

/* Function: lp_compile
 * Compile some tinypy code.
 *
 */
lp_obj* lp_compile(LP, lp_obj* text, lp_obj* fname)
{
	const char* r;
	int size, result;
	lp_obj* code;

	if (text->type != LP_STRING || fname->type != LP_STRING)
	{
		lp_raise(0, lp_string(lp, "(lp_compile) expected string"));
	}
    r = compile(fname->string.val, text->string.val, &size, &result);
	if (!result)
	{
		lp_obj* m = lp_string_copy(lp, r, strlen(r));
		free(r);
		lp_raise(0, m);
	}
	code = lp_string_copy(lp, r, size);
	free(r);
	return code;
}

/* Function: lp_exec
 * Execute VM code.
 */
lp_obj* lp_exec(LP, lp_obj* code, lp_obj* globals)
{
    lp_obj* r = lp->lp_None;
    lp_frame(lp,globals,code,&r);
	if (!lp_run(lp, lp->cur)) return 0;
	RETURN_LP_OBJ(r);
}

lp_obj* lpf_exec(LP)
{
    lp_obj* code = LP_OBJ(0);
    lp_obj* globals = LP_OBJ(1);
    lp_obj* r = lp->lp_None;
    lp_frame(lp,globals,code,&r);
	if (!lp_run(lp, lp->cur)) return 0;
	RETURN_LP_OBJ(r);
}


lp_obj* lpf_import(LP)
{
    lp_obj* mod = LP_OBJ(0);
	lp_obj* suffix, *fn, *r;

    if (lp_has(lp,lp->modules,mod)->integer) {
        return lp_get(lp,lp->modules,mod);
    }
    
	suffix = lp_string(lp, ".py");
	fn = lp_add(lp, mod, suffix);
	LP_OBJ_DEC(suffix);
	if (!fn) return 0;
    r = lp_import(lp,fn,mod,lp->lp_None);
	LP_OBJ_DEC(fn);
    return r;
}

lp_obj* lpf_compile(LP)
{
    lp_obj* text = LP_OBJ(0);
    lp_obj* fname = LP_OBJ(1);
    return lp_compile(lp, text, fname);
}

lp_obj* lpf_eval(LP)
{
    lp_obj* text = LP_OBJ(0);
    lp_obj* globals = LP_OBJ(1);
	lp_obj* evn = lp_string(lp, "<eval>");
    lp_obj* code = lp_compile(lp, text, evn);
	LP_OBJ_DEC(evn);
	if (!code) return 0;
    return lp_exec(lp, code, globals);
}

void lp_builtins(LP) {
    lp_obj* o;
    struct {const char *s;void *f;} b[] = {
    {"print",lpf_print}, {"range",lpf_range}, {"min",lpf_min},
    {"max",lpf_max}, {"bind",lpf_bind}, {"copy",lpf_copy},
    {"import",lpf_import}, {"len",lpf_len}, {"assert",lpf_assert},
    {"str",lpf_str2}, {"float",lpf_float}, {"system",lpf_system},
    {"istype",lpf_istype}, {"chr",lpf_chr}, {"save",lpf_save},
    {"load",lpf_load}, {"fpack",lpf_fpack}, {"abs",lpf_abs},
    {"int",lpf_int}, {"exec",lpf_exec}, {"exists",lpf_exists},
    {"mtime",lpf_mtime}, {"number",lpf_float}, {"round",lpf_round},
    {"ord",lpf_ord}, {"merge",lpf_merge}, {"getraw",lpf_getraw},
    {"setmeta",lpf_setmeta}, {"getmeta",lpf_getmeta},
    {"bool", lpf_builtins_bool},
    {"compile",lpf_compile},  {"eval",lpf_eval}, {"disasm",lpf_disasm},
    #ifdef LP_SANDBOX
    {"sandbox",lp_sandbox_},
    #endif
    {0,0},
    };
    int i; for(i=0; b[i].s; i++) {
        lp_set(lp,lp->builtins,lp_string(lp, b[i].s),lp_fnc(lp,(lp_obj *(*)(lp_vm *))b[i].f));
    }
    
    o = lp_object(lp);
    lp_setkv(lp,o,lp_string(lp, "__call__"),lp_fnc(lp,lpf_object_call));
    lp_setkv(lp,o,lp_string(lp, "__new__"),lp_fnc(lp,lpf_object_new));
    lp_setk(lp,lp->builtins,lp_string(lp, "object"),o);
}


void lp_args(LP,int argc, char *argv[]) {
    lp_obj* self = lp_list(lp), *v;
    int i;
    for (i=1; i<argc; i++)
	{
		lp_obj* v = lp_string(lp, argv[i]);
		_lp_list_append(lp,self->list,v);
		LP_OBJ_DEC(v);
	}
	v = lp_string(lp, "ARGV");
    lp_set(lp,lp->builtins,v,self);
	LP_OBJ_DEC(v);
}

lp_obj* lp_main(LP,char *fname, void *code, int len) {
    return lp_import_(lp,fname,"__main__",code, len);
}

lp_obj* lp_eval(LP, const char *text, lp_obj* globals) {
	lp_obj* t = lp_string(lp, text);
	lp_obj* e = lp_string(lp, "<eval>");
    lp_obj* code = lp_compile(lp,t,e);
	LP_OBJ_DEC(t);
	LP_OBJ_DEC(e);
	if (!code) return 0;
    lp_obj* r = lp_exec(lp,code,globals);
	LP_OBJ_DEC(code);
	return r;
}

/* Function: lp_init
 * Initializes a new virtual machine.
 *
 * The given parameters have the same format as the parameters to main, and
 * allow passing arguments to your tinypy scripts.
 *
 * Returns:
 * The newly created tinypy instance.
 */
lp_vm *lp_init(int argc, char *argv[]) {
    lp_vm *lp = _lp_init();
    lp_builtins(lp);
    math_init(lp);
    random_init(lp);
    re_init(lp);
	time_init(lp);
    lp_args(lp,argc,argv);
    return lp;
}

/**/
