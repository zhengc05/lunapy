#include <stdio.h>
#include "opcode.h"
#include "sym.h"
#include "mem.h"
#include "tokenize.h"
#include <stdlib.h>

typedef unsigned char REG_TYPE;
const REG_TYPE INVALID_REG = 255;

enum TagTyoe
{
	TAG_END,
	TAG_SETJMP,
};

void insert(struct CompileState *c, struct Item* v)
{
	append_itemList_item(&c->D.out, v);
}

void code(struct CompileState *cst, char i, char a, char b, char c)
{
	struct Item* t = new_code_item(cst, I_CODE);
	t->d[0] = i;
	t->d[1] = a;
	t->d[2] = b;
	t->d[3] = c;
	t->next = 0;
	insert(cst, t);
}

void begin(struct CompileState *cst, bool gbl)
{
	struct Scope *sco = (struct Scope *)malloc(sizeof(struct Scope));
	memset(sco, 0, sizeof(struct Scope));
	sco->next = cst->D.scope;
	if (cst->D.scope)
	{
		sco->snum = cst->D.scope->_scopei;
		sco->_scopei = cst->D.scope->_scopei + 1;
	}
	else {
		sco->snum = 0;
		sco->_scopei = 1;
	}
	sco->cregs = new_code_item(cst, I_REGS);
	sco->_globals = gbl;
	cst->D.scope = sco;
	insert(cst, sco->cregs);
}

void end(struct CompileState *cst)
{
	struct Scope* sco;
	cst->D.scope->cregs->d[0] = cst->D.scope->mreg;
	code(cst, OP_EOF, 0, 0, 0);

	if (cst->D.scope->tmpc != 0)
		printf("Warning:\nencode.py contains a register leak\n");

	sco = cst->D.scope;
	cst->D.scope = cst->D.scope->next;
	free(sco);
}

REG_TYPE n2r(struct CompileState *c, const char* n)
{
	for (int r = 0; r < 255; r ++)
	{
		if (c->D.scope->r2n[r] && !strcmp(n, c->D.scope->r2n[r]))
			return r;
	}
	return INVALID_REG;
}

void write(struct CompileState *c, const char* v, int len)
{
	int i = 0;
	while (i < len)
	{
		int l = 0;
		struct Item* t = new_code_item(c, I_DATA);
		while (l < 4 && (i+l) < len)
		{
			t->d[l] = v[i+l];
			l ++;
		}
		while (l < 4)
		{
			t->d[l] = 0;
			l ++;
		}
		insert(c, t);
		i += l;
	}
}


void code_16(struct CompileState *c, char i, char a, short b)
{
	code(c, i, a, (b&0xff00)>>8, (b&0xff)>>0);
}

void get_code_16(struct Item* item, char i, char a, short b)
{
	item->isdata = I_CODE;
	item->d[0] = i;
	item->d[1] = a;
	item->d[2] = (b&0xff00)>>8;
	item->d[3] = (b&0xff)>>0;
}

void selpos(struct CompileState *c, int row)
{
	int len;
	const char* text;
	if (row == c->D.scope->lineno) return;
	text = c->D.lines[row-1];
	c->D.scope->lineno = row;
	len = strlen(text) + 4 - strlen(text)%4;
	code_16(c, OP_POS, len/4, row);
	write(c, text, len);
}

void alloc(struct CompileState *c, int t, REG_TYPE* st, REG_TYPE* end)
{
	int m = c->D.scope->mreg + t;
	if (m > 256) m = 256;
	for(int i = 0; i < m; i++)
	{
		if (!c->D.scope->r2n[i])
		{
			int j = 0;
			for(; j < t; j ++)
			{
				if (c->D.scope->r2n[i+j])
				{
					break;
				}
			}
			if (j == t)
			{
				*st = i;
				*end = i+t;
				return;
			}else{
				i += j;
			}
		}
	}
}

void set_reg(struct CompileState *c, REG_TYPE r, char* n)
{
	c->D.scope->r2n[r] = n;
	c->D.scope->mreg = max(c->D.scope->mreg,r+1);
}

int get_reg(struct CompileState *c, char* n)
{
	REG_TYPE r = n2r(c, n);
	if (r == INVALID_REG)
	{
		REG_TYPE st, end;
		alloc(c, 1, &st, &end);
		set_reg(c, st, n);
		return st;
	}

	return r;
}

bool is_tmp(struct CompileState *c, REG_TYPE r)
{
	if (!c->D.scope->r2n[r]) return false;
	return c->D.scope->r2n[r][0] == '$';
}

void free_reg(struct CompileState *c, REG_TYPE r)
{
	if (is_tmp(c, r)) c->D.scope->tmpc --;
	c->D.scope->r2n[r] = 0;
}

void get_tmps(struct CompileState *c, int t, REG_TYPE* st, REG_TYPE* end)
{
	alloc(c, t, st, end);
	for(REG_TYPE r = *st; r < *end; r ++)
	{
		char *s = new_string(c, 5);
		snprintf(s, 5, "$%d", c->D.scope->_tmpi);
		set_reg(c, r, s);
		c->D.scope->_tmpi ++;
	}
	c->D.scope->tmpc += t;
}

int get_tmp(struct CompileState *c, REG_TYPE r)
{
	REG_TYPE st, end;

	if (r != INVALID_REG) return r;
	get_tmps(c, 1, &st, &end);
	return st;
}

void un_tmp(struct CompileState *c, REG_TYPE r)
{
	char* n = c->D.scope->r2n[r], *s;
	free_reg(c, r);
	s = new_string(c, strlen(n) + 2);
	s[0] = '*';
	strcpy(&s[1], n);
	set_reg(c, r, s);
}

int free_tmp(struct CompileState *c, REG_TYPE r)
{
	if (is_tmp(c, r)) free_reg(c, r);
	return r;
}

void free_tmps(struct CompileState *c, REG_TYPE st, REG_TYPE end)
{
	for (REG_TYPE k = st; k < end; k ++) free_tmp(c, k);
}

REG_TYPE _do_string(struct CompileState *c, const char* v, int r)
{
	r = get_tmp(c, r);
	int len = strlen(v) + (4-strlen(v)%4);
	code_16(c, OP_STRING, r, strlen(v));
	write(c, v, len);
	return r;
}

REG_TYPE do_string(struct CompileState *c, struct Token* t, REG_TYPE r)
{
	if (t->type == S_SYMBOL)
		return _do_string(c, CONSTR[t->vs], r);
	else
		return _do_string(c, t->vn, r);
}

REG_TYPE _do_number(struct CompileState *c, char* v, int r)
{
	r = get_tmp(c, r);
	if (strstr(v, "."))
	{
		double f = atof(v);
		char *t = (char *)malloc(sizeof(double));
		code(c, OP_NUMBER, r, 1, 0);
		memcpy(t, &f, sizeof(double));
		write(c, t, sizeof(double));
		free(t);
	}
	else {
		int i = atoi(v);
		char *t = (char *)malloc(sizeof(int));
		code(c, OP_NUMBER, r, 0, 0);
		memcpy(t, &i, sizeof(int));
		write(c, t, sizeof(int));
		free(t);
	}
	return r;
}

REG_TYPE do_number(struct CompileState *c, struct Token* t, REG_TYPE r)
{
	return _do_number(c, t->vn, r);
}

int get_tag(struct CompileState *c)
{
	int k = c->D._tagi;
	c->D._tagi ++;
	return k;
}

int stack_tag(struct CompileState *c)
{
	int k = get_tag(c);
	struct IntListItem* i = new_intList_item(c, k);
	i->next = c->D.tstack;
	c->D.tstack = i;
	return k;
}

void pop_tag(struct CompileState *c)
{
	c->D.tstack = c->D.tstack->next;
}

void tag(struct CompileState *c, int t, int s)
{
	struct Item* item = new_code_item(c, I_TAG);
	item->d[0] = t;
	item->d[1] = s;
	item->d[2] = c->D.scope->snum;
	insert(c, item);
}

void jump(struct CompileState *c, int t, int s)
{
	struct Item* item = new_code_item(c, I_JUMP);
	item->d[0] = t;
	item->d[1] = s;
	item->d[2] = c->D.scope->snum;
	insert(c, item);
}

void setjump(struct CompileState *c, int t, int s)
{
	struct Item* item = new_code_item(c, I_SETJMP);
	item->d[0] = t;
	item->d[1] = s;
	item->d[2] = c->D.scope->snum;
	insert(c, item);
}

REG_TYPE fnc(struct CompileState *c, int t, int s)
{
	char tmp[256];
	struct Item* item = new_code_item(c, I_FNC);
	REG_TYPE r;
	int n = snprintf(tmp, 256, "%d:%d:%d", c->D.scope->snum, t, s);
	char* vs = new_string(c, n+1);
	memcpy(vs, tmp, n+1);
	r = get_reg(c, vs);
	item->d[0] = t;
	item->d[1] = s;
	item->d[2] = c->D.scope->snum;
	item->d[3] = r;
	insert(c, item);
	return r;
}

short find_tag(struct ItemList* lp, unsigned char t, unsigned char s, unsigned char i)
{
	struct Item*p = lp->head;
	while(p)
	{
		if (p->d[0] == t && p->d[1] == s && p->d[2] == i)
			return p->tag_id;
		p = p->next;
	}
	return 0;
}

void merge_tags(struct CompileState *c)
{
	int n = 0, t;
	struct Item* p = c->D.out.head, *o = c->D.out.head, *tmp;
	struct ItemList lpp = { 0 };

	c->D.out.head = 0;
	c->D.out.num = 0;
	c->D.out.tail = 0;
	while(p)
	{
		if (p->isdata == I_TAG)
		{
			tmp = p;
			tmp->tag_id = n;
			p = p->next;
			append_itemList_item(&lpp, tmp);
			continue;
		}
		else if(p->isdata == I_REGS)
		{
			code_16(c, OP_REGS, p->d[0], 0);
			p = p->next;
			n ++;
			continue;
		}
		tmp = p;
		p = p->next;
		insert(c, tmp);
		n ++;
	}

	n = 0;
	p = c->D.out.head;
	while(p)
	{
		switch (p->isdata)
		{
		case I_JUMP:
			get_code_16(p, OP_JUMP, 0, find_tag(&lpp, p->d[0], p->d[1], p->d[2]) - n);
			break;
		case I_SETJMP:
			get_code_16(p, OP_SETJMP, 0, find_tag(&lpp, p->d[0], p->d[1], p->d[2]) - n);
			break;
		case I_FNC:
			t = find_tag(&lpp, p->d[0], p->d[1], p->d[2]);
			get_code_16(p, OP_DEF, p->d[3], t - n);
			break;
		}
		p = p->next;
		n ++;
	}

	n = 0;
	p = c->D.out.head;
	while(p)
	{
		switch (p->isdata)
		{
		case I_DATA:
			n += 4;
			break;
		case I_CODE:
			n += 4;
			break;
		}
		p = p->next;
	}

	c->D.so = new_string(c, n);
	c->D.nso = n;
	n = 0;
	p = c->D.out.head;
	while(p)
	{
		switch (p->isdata)
		{
		case I_DATA:
			memcpy(&c->D.so[n], p->d, 4);
			n += 4;
			break;
		case I_CODE:
			memcpy(&c->D.so[n], p->d, 4);
			n += 4;
			break;
		default:
			u_error(c, "huh?", "", 0, 0);
			break;
		}
		p = p->next;
	}
}

REG_TYPE do_symbol(struct CompileState *cst, struct Token* t, REG_TYPE r);

REG_TYPE imanage(struct CompileState *cst, struct Token* orig)
{
	struct Token* t = new_token(cst, orig->col, orig->row, S_SYMBOL, S_EQUAL);
	append_list_item(cst, &t->items, orig->items.head->t);
	append_list_item(cst, &t->items, orig);
	orig->vs = in_symbol(CONSTR[orig->vs], strlen(CONSTR[orig->vs])-1);
	return do_symbol(cst, t, INVALID_REG);
}

REG_TYPE do_expression(struct CompileState *cst, struct Token* t, REG_TYPE r);

REG_TYPE unary(struct CompileState *cst, int i, struct Token* tb, REG_TYPE r)
{
	REG_TYPE b;
	r = get_tmp(cst, r);
	b = do_expression(cst, tb, INVALID_REG);
	code(cst, i, r, b, 0);
	if (r != b) free_tmp(cst, b);
	return r;
}

REG_TYPE infix(struct CompileState *cst, int i, struct Token* tb, struct Token* tc, REG_TYPE r)
{
	REG_TYPE b, c;
	r = get_tmp(cst, r);
	b = do_expression(cst, tb, r);
	c = do_expression(cst, tc, INVALID_REG);
	code(cst, i, r, b, c);
	if (r != b) free_tmp(cst, b);
	free_tmp(cst, c);
	return r;
}

REG_TYPE logic_infix(struct CompileState *cst, int op, struct Token* tb, struct Token* tc, REG_TYPE _r)
{
	int t = get_tag(cst);
	REG_TYPE r = do_expression(cst, tb, _r);
	if (_r != r) free_tmp(cst, _r);
	if (op == S_AND)   code(cst, OP_IF, r, 0, 0);
	else if (op == S_OR)  code(cst, OP_IFN, r, 0, 0);
	jump(cst, t, TAG_END);
	_r = r;
	r = do_expression(cst, tc, _r);
	if (_r != r) free_tmp(cst, _r);
	tag(cst, t, TAG_END);
	return r;
}

REG_TYPE _do_none(struct CompileState *c, REG_TYPE r)
{
	r = get_tmp(c, r);
	code(c, OP_NONE, r, 0, 0);
	return r;
}

int g_sets[] = {S_EQUAL};
int g_isets[] = {S_PLUSEQUAL, S_MINUSEQUAL, S_STAREQUAL, S_DIVEQUAL, S_OREQUAL, S_ANDEQUAL, S_UPEQUAL};
int g_cmps[] = {S_LOW, S_BIG, S_LOWEQUAL, S_BIGEQUAL, S_EQUALTWO, S_NOTEQUAL};

static int _get_metas(int sym)
{
	switch (sym)
	{
	case S_PLUS:
		return OP_ADD;
	case S_STAR:
		return OP_MUL;
	case S_DIV:
		return OP_DIV;
	case S_STARTWO:
		return OP_POW;
	case S_MINUS:
		return OP_SUB;
	case S_CENT:
		return OP_MOD;
	case S_BIGTWO:
		return OP_RSH;
	case S_LOWTWO:
		return OP_LSH;
	case S_AND:
		return OP_BITAND;
	case S_OR:
		return OP_BITOR;
	case S_UP:
		return OP_BITXOR;
	}
}

REG_TYPE do_set_ctx(struct CompileState *cst, struct Token* k, struct Token* v);

REG_TYPE do_symbol(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	struct TListItem* items = t->items.head;

	if (t->vs == S_NONE) return _do_none(cst, r);
	if (t->vs == S_TRUE)
		return _do_number(cst, "1", r);
	if (t->vs == S_FALSE)
		return _do_number(cst, "0", r);

	if (t->vs == S_AND || t->vs == S_OR)
		return logic_infix(cst, t->vs, items->t, items->next->t, r);
	if (in_sets(g_isets, sizeof(g_isets) / sizeof(int), t->vs))
		return imanage(cst, t);
	if (t->vs == S_IS)
		return infix(cst, OP_EQ, items->t, items->next->t, r);
	if (t->vs == S_ISNOT)
		return infix(cst, OP_CMP, items->t, items->next->t, r);
	if (t->vs == S_NOT)
		return unary(cst, OP_NOT, items->t, r);
	if (t->vs == S_IN)
		return infix(cst, OP_HAS, items->next->t, items->t, r);
	if (t->vs == S_NOTIN)
	{
		REG_TYPE zero;
		r = infix(cst, OP_HAS, items->next->t, items->t, r);
		zero = _do_number(cst, "0", INVALID_REG);
		code(cst, OP_EQ, r, r, free_tmp(cst, zero));
		return r;
	}
	if (in_sets(g_sets, sizeof(g_sets) / sizeof(int), t->vs))
		return do_set_ctx(cst, items->t, items->next->t);
	else if (in_sets(g_cmps, sizeof(g_cmps) / sizeof(int), t->vs))
	{
		struct Token* b = items->t, *c = items->next->t;
		int cd = OP_EQ;
		int v = t->vs;
		if (v == S_BIG || v == S_BIGEQUAL)
		{
			b = items->next->t;
			c = items->t;

			if (v == S_BIG) v = S_LOW;
			else v = S_LOWEQUAL;
		}

		if (v == S_LOW) cd = OP_LT;
		if (v == S_LOWEQUAL) cd = OP_LE;
		if (v == S_NOTEQUAL) cd = OP_NE;
		return infix(cst, cd, b, c, r);
	}
	else
		return infix(cst, _get_metas(t->vs), items->t, items->next->t, r);
}

REG_TYPE do_local(struct CompileState *cst, struct Token* t, REG_TYPE r);

REG_TYPE do_set_ctx(struct CompileState *cst, struct Token* k, struct Token* v)
{
	REG_TYPE r, tmp, rr;

	if (k->type == S_NAME)
	 {
		REG_TYPE a, b, c;
		if ((cst->D.scope->_globals && !find_str_item(&cst->D.scope->vars, k->vn)) || find_str_item(&cst->D.scope->globals, k->vn))
		{
			c = do_string(cst, k, INVALID_REG);
			b = do_expression(cst, v, INVALID_REG);
			code(cst, OP_GSET, c, b, 0);
			free_tmp(cst, c);
			free_tmp(cst, b);
			return INVALID_REG;
		}
		a = do_local(cst, k, INVALID_REG);
		b = do_expression(cst, v, INVALID_REG);
		code(cst, OP_MOVE, a, b, 0);
		free_tmp(cst, b);
		return a;
	}
	else if (k->type  == S_TUPLE || k->type == S_LIST)
	{
		struct Token* vv, *nt;
		struct TListItem* kk;
		int n, l;

		if ((v & 0x7)  == S_TUPLE || (v & 0x7) == S_LIST)
		{
			struct IntList tmps = { 0 };
			struct IntListItem *lp;
			kk = k->items.head;
			while (kk)
			{
				vv = kk->t;
				tmp = get_tmp(cst, INVALID_REG);
				append_intList_item(cst, &tmps, tmp);
				r = do_expression(cst, vv, INVALID_REG);
				code(cst, OP_MOVE, tmp, r, 0);
				free_tmp(cst, r);
				kk = kk->next;
			}
			kk = k->items.head;
			lp = tmps.head;
			while (kk)
			{
				vv = kk->t;
				tmp = lp->v;
				nt = new_token(cst, vv->col, vv->row, S_REG, tmp);
				free_tmp(cst, do_set_ctx(cst, vv, nt));
				kk = kk->next;
				lp = lp->next;
			}
			return INVALID_REG;
		}

		r = do_expression(cst, v, INVALID_REG);
		un_tmp(cst, r);
		nt = new_token(cst, v->col, v->row, S_REG, r);
		kk = k->items.head;
		n = 0;
		while (kk)
		{
			char buffer[8];
			struct Token* nt1 = new_token(cst, nt->col, nt->row, S_NUMBER, 0);
			struct Token* nt2 = new_token(cst, nt->col, nt->row, S_GET, 0);
			l = snprintf(buffer, 8, "%d", n);
			nt1->vn = new_string(cst, l+1);
			memcpy(nt1->vn, buffer, l+1);
			append_list_item(cst, &nt2->items, nt);
			append_list_item(cst, &nt2->items, nt1);
			free_tmp(cst, do_set_ctx(cst, kk->t, nt2));
			kk = kk->next;
			n ++;
		}
		free_reg(cst, r);
		return INVALID_REG;
	}
	r = do_expression(cst, k->items.head->t, INVALID_REG);
	rr = do_expression(cst, v, INVALID_REG);
	tmp = do_expression(cst, k->items.head->next->t, INVALID_REG);
	code(cst, OP_SET, r, tmp, rr);
	free_tmp(cst, r);
	free_tmp(cst, tmp);
	return rr;
}

REG_TYPE manage_seq(struct CompileState *cst, int i, REG_TYPE a, struct TList* items, int sav)
{
	REG_TYPE st, end, n;
	struct TListItem* tt;
	int l = get_list_len(items);
	if (l < sav) l = sav;
	get_tmps(cst, l, &st, &end);
	n = st;
	tt = items->head;
	while (tt)
	{
		REG_TYPE r = n;
		REG_TYPE b = do_expression(cst, tt->t, r);  //  
		if (r != b)
		{
			code(cst, OP_MOVE, r, b, 0);
			free_tmp(cst, b);
		}
		tt = tt->next;
		n ++;
	}
	if (st == end)
	{
		code(cst, i, a, 0, 0);
		return INVALID_REG;
	}
	code(cst, i, a, st, get_list_len(items));
	free_tmps(cst, st+sav, end);
	return st;
}

/*
 *     a  --> args
 *     b  --> keyword args
 *     c  --> *args
 *     d  --> **args
 */
void p_filter(struct CompileState *cst, struct TListItem* items, struct TList* a, struct TList* b, struct Token** c, struct Token** d)
{
	*c = 0;
	*d = 0;
	while (items)
	{
		struct Token* t = items->t;
		if (t->type == S_SYMBOL && t->vs == S_EQUAL) append_list_item(cst, b, t);
		else if (t->type == S_ARGS) *c = t;
		else if (t->type == S_NARGS) *d = t;
		else append_list_item(cst, a, t);
		items = items->next;
	}
}

REG_TYPE do_call(struct CompileState *cst, struct Token* t, REG_TYPE r);

REG_TYPE do_import(struct CompileState *cst, struct Token* t)
{
	REG_TYPE v;
	struct TListItem* k = t->items.head;
	while (k)
	{
		struct Token* mod = k->t, *nt, *nt2;
		mod->type = S_STRING;
		nt = new_token(cst, t->col, t->row, S_CALL, 0);
		nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
		nt2->vn = "import";
		append_list_item(cst, &nt->items, nt2);
		append_list_item(cst, &nt->items, mod);
		v = do_call(cst, nt, INVALID_REG);
		mod->type = S_NAME;
		nt = new_token(cst, t->col, t->row, S_REG, v);
		do_set_ctx(cst, mod, nt);
		k = k->next;
	}
	return INVALID_REG;
}

REG_TYPE do_from(struct CompileState *cst, struct Token* t)
{
	REG_TYPE v;
	struct Token* mod, *item, *nt, *nt2, *nt3, *nt4;

	mod = t->items.head->t;
	mod->type = S_STRING;
	nt = new_token(cst, t->col, t->row, S_CALL, 0);
	nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
	nt2->vn = "import";
	append_list_item(cst, &nt->items, nt2);
	append_list_item(cst, &nt->items, mod);
	v = do_expression(cst, nt, INVALID_REG);
	item = t->items.head->next->t;
	if (item->vs == S_STAR)
	{
		nt = new_token(cst, t->col, t->row, S_CALL, 0);
		nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
		nt3 = new_token(cst, t->col, t->row, S_NAME, 0);
		nt4 = new_token(cst, t->col, t->row, S_REG, v);
		nt2->vn = "merge";
		nt3->vn = "__dict__";
		append_list_item(cst, &nt->items, nt2);
		append_list_item(cst, &nt->items, nt3);
		append_list_item(cst, &nt->items, nt4);
		free_tmp(cst, do_expression(cst, nt, INVALID_REG));
	}
	else
	{
		item->type = S_STRING;
		nt = new_token(cst, t->col, t->row, S_GET, 0);
		nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
		nt3 = new_token(cst, t->col, t->row, S_GET, 0);
		nt4 = new_token(cst, t->col, t->row, S_REG, v);
		nt2->vn = "__dict__";
		append_list_item(cst, &nt->items, nt2);
		append_list_item(cst, &nt->items, item);
		append_list_item(cst, &nt3->items, nt4);
		append_list_item(cst, &nt3->items, item);
		free_tmp(cst, do_set_ctx(cst, nt, nt3));
	}

	return INVALID_REG;
}

REG_TYPE do_globals(struct CompileState *cst, struct Token* t)
{
	struct TListItem* kk = t->items.head;
	while (kk)
	{
		t = kk->t;
		if (!find_str_item(&cst->D.scope->globals, t->vn))
			append_strList_item(cst, &cst->D.scope->globals, t->vn);
		kk = kk->next;
	}

	return INVALID_REG;
}

REG_TYPE do_del(struct CompileState *cst, struct Token* tt)
{
	REG_TYPE r, r2;
	struct TListItem* kk = tt->items.head;
	while (kk)
	{
		struct Token* t = kk->t;
		r = do_expression(cst, t->items.head->t, INVALID_REG);
		r2 = do_expression(cst, t->items.head->next->t, INVALID_REG);
		code(cst, OP_DEL, r, r2, 0);
		free_tmp(cst, r);
		free_tmp(cst, r2);
	}

	return INVALID_REG;
}

/*
 * call
 *   |
 *   |-- func  -->  fnc
 *   |-- arg1
 *   |-- arg2
 *   ...
 */
REG_TYPE do_call(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	REG_TYPE fnc = do_expression(cst, t->items.head->t, INVALID_REG);
	struct TList a = { 0 }, b = { 0 };
	struct TListItem * item;
	struct Token *c, *d, *nt, *nt2, *nt3;
	REG_TYPE e, t1, t2;
	
	r = get_tmp(cst, r);
	p_filter(cst, t->items.head->next, &a, &b, &c, &d);
	e = INVALID_REG;
	if (b.num != 0 || d != 0)
	{
		e = do_expression(cst, new_token(cst, t->col, t->row, S_DICT, 0), INVALID_REG);
		un_tmp(cst, e);
		item = b.head;
		while (item)
		{
			struct Token* p = item->t;
			p->items.head->t->type = S_STRING;
			t1 = do_expression(cst, p->items.head->t, INVALID_REG);
			t2 = do_expression(cst, p->items.head->next->t, INVALID_REG);
			code(cst, OP_SET, e, t1, t2);
			free_tmp(cst, t1);
			free_tmp(cst, t2);
			item = item->next;
		}
		if (d)
		{
			nt = new_token(cst, t->col, t->row, S_CALL, 0);
			nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
			nt3 = new_token(cst, t->col, t->row, S_REG, e);
			nt2->vn = "merge";
			append_list_item(cst, &nt->items, nt2);
			append_list_item(cst, &nt->items, nt3);
			append_list_item(cst, &nt->items, d->items.head->t);
			free_tmp(cst, do_expression(cst, nt, INVALID_REG));
		}
	}
	manage_seq(cst, OP_PARAMS, r, &a, 0);
	if (c)
	{
		t1 = _do_string(cst, "*", INVALID_REG);
		t2 = do_expression(cst, c->items.head->t, INVALID_REG);
		code(cst, OP_SET, r, t1, t2);
		free_tmp(cst, t1);
		free_tmp(cst, t2);
	}
	if (e != INVALID_REG)
	{
		REG_TYPE t1 = _do_none(cst, INVALID_REG);
		code(cst, OP_SET, r, t1, e);
		free_tmp(cst, t1);
	}
	code(cst, OP_CALL, r, fnc, r);
	free_tmp(cst, fnc);
	return r;
}

REG_TYPE do_name(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	REG_TYPE c;
	if (find_str_item(&cst->D.scope->vars, t->vn))
		return do_local(cst, t, r);
	if (!find_str_item(&cst->D.scope->rglobals, t->vn))
		append_strList_item(cst, &cst->D.scope->rglobals, t->vn);
	r = get_tmp(cst, r);
	c = do_string(cst, t, INVALID_REG);
	code(cst, OP_GGET, r, c, 0);
	free_tmp(cst, c);
	return r;
}

REG_TYPE do_local(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	if (find_str_item(&cst->D.scope->rglobals, t->vn))
		u_error(cst, "UnboundLocalError", cst->D.code, t->col, t->row);
	if (!find_str_item(&cst->D.scope->vars, t->vn))
		append_strList_item(cst, &cst->D.scope->vars, t->vn);
	return get_reg(cst, t->vn);
}

void do_info(struct CompileState *cst, const char* name)
{
	//if '-nopos' in ARGV: return
	code(cst, OP_FILE, free_tmp(cst, _do_string(cst, cst->D.fname, INVALID_REG)), 0, 0);
	code(cst, OP_NAME, free_tmp(cst, _do_string(cst, name, INVALID_REG)), 0, 0);
}

REG_TYPE do_def(struct CompileState *cst, struct Token* tok, REG_TYPE kls)
{
	int t = get_tag(cst);
	REG_TYPE rf = fnc(cst, t, S_END), rn, r, v, e, tmp;
	struct TList a = { 0 }, b = { 0 };
	struct TListItem *item;
	struct Token *c, *d, *nt;

	begin(cst, false);
	selpos(cst, tok->row);
	nt = new_token(cst, tok->col, tok->row, S_NAME, 0);
	nt->vn = "__params";
	r = do_local(cst, nt, INVALID_REG);
	do_info(cst, tok->items.head->t->vn);
	p_filter(cst, tok->items.head->next->t->items.head, &a, &b, &c, &d);
	item = a.head;
	while (item)
	{
		struct Token* p = item->t;
		v = do_local(cst, p, INVALID_REG);
		tmp = _do_none(cst, INVALID_REG);
		code(cst, OP_GET, v, r, tmp);
		free_tmp(cst, tmp);
		item = item->next;
	}
	item = b.head;
	while (item)
	{
		struct Token* p = item->t;
		v = do_local(cst, p->items.head->t, INVALID_REG);
		do_expression(cst, p->items.head->next->t, v);
		tmp = _do_none(cst, INVALID_REG);
		code(cst, OP_IGET, v, r, tmp);
		free_tmp(cst, tmp);
		item = item->next;
	}
	if (c)
	{
		v = do_local(cst, c->items.head->t, INVALID_REG);
		tmp = _do_string(cst, "*", INVALID_REG);
		code(cst, OP_GET, v, r, tmp);
		free_tmp(cst, tmp);
	}
	if (d)
	{
		e = do_local(cst, d->items.head->t, INVALID_REG);
		code(cst, OP_DICT, e, 0, 0);
		tmp = _do_none(cst, INVALID_REG);
		code(cst, OP_IGET, e, r, tmp);
		free_tmp(cst, tmp);
	}
	free_tmp(cst, do_expression(cst, tok->items.head->next->next->t, INVALID_REG));
	end(cst);

	tag(cst, t, S_END);

	if (kls == INVALID_REG)
	{
		if (cst->D.scope->_globals)
		{
			nt = new_token(cst, tok->col, tok->row, 0, 0);
			append_list_item(cst, &nt->items, tok->items.head->t);
			do_globals(cst, nt);
		}
		nt = new_token(cst, tok->col, tok->row, S_REG, rf);
		r = do_set_ctx(cst, tok->items.head->t, nt);
	}
	else
	{
		rn = do_string(cst, tok->items.head->t, INVALID_REG);
		code(cst, OP_SET, kls, rn, rf);
		free_tmp(cst, rn);
	}

	free_tmp(cst, rf);

	return INVALID_REG;
}

REG_TYPE do_def_wrapper(struct CompileState *cst, struct Token* tok)
{
	return do_def(cst, tok, INVALID_REG);
}
  
REG_TYPE do_classvar(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	REG_TYPE var = do_string(cst, t->items.head->t, INVALID_REG);
	REG_TYPE val = do_expression(cst, t->items.head->next->t, INVALID_REG);
	code(cst, OP_SET, r, var, val);
	free_reg(cst, var);
	free_reg(cst, val);
	return INVALID_REG;
}

/*
 *      class
 *        |-- name
 *        |     |-- name
 *        |     |-- parent
 *        |-- var
 *        |-- def
 *         ...
 */
REG_TYPE do_class(struct CompileState *cst, struct Token* t)
{
	struct TListItem* item = t->items.head, *kk;
	struct Token* parent = 0, *nt, *nt2, *nt3;
	const char* name;
	REG_TYPE kls, ts;
	if (item->t->type == S_NAME)
	{
		name = item->t->vn;
		parent = new_token(cst, t->col, t->row, S_NAME, 0);
		parent->vn = "object";
	}
	else
	{
		name = item->t->items.head->t->vn;
		parent = item->t->items.head->next->t;
	}

	kls = do_expression(cst, new_token(cst, t->col, t->row, S_DICT, 0), INVALID_REG);
	un_tmp(cst, kls);
	ts = _do_string(cst, name, INVALID_REG);
	code(cst, OP_GSET, ts, kls, 0);
	free_tmp(cst, ts);
	
	nt = new_token(cst, t->col, t->row, S_CALL, 0);
	nt2 = new_token(cst, t->col, t->row, S_NAME, 0);
	nt3 = new_token(cst, t->col, t->row, S_REG, kls);
	nt2->vn = "setmeta";
	append_list_item(cst, &nt->items, nt2);
	append_list_item(cst, &nt->items, nt3);
	append_list_item(cst, &nt->items, parent);
	free_tmp(cst, do_expression(cst, nt, INVALID_REG));
		
	kk = t->items.head->next->t->items.head;
	while (kk)
	{
		struct Token* member = kk->t;
		if (member->type == S_DEF) do_def(cst, member, kls);
		else if (member->type == S_SYMBOL && member->vs == S_EQUAL) do_classvar(cst, member, kls);
		kk = kk->next;
	}
		
	free_reg(cst, kls);

	return INVALID_REG;
}

/*
 *    while
 *      |-- cond
 *      |-- block
 */
REG_TYPE do_while(struct CompileState *cst, struct Token* tok)
{
	REG_TYPE r;
    int t = stack_tag(cst);
    tag(cst, t, S_BEGIN);
    tag(cst, t, S_CONTINUE);
    r = do_expression(cst, tok->items.head->t, INVALID_REG);
    code(cst, OP_IF, r, 0, 0);
    free_tmp(cst, r);
    jump(cst, t, S_END);
    free_tmp(cst, do_expression(cst, tok->items.head->next->t, INVALID_REG));
    jump(cst, t, S_BEGIN);
    tag(cst, t, S_BREAK);
    tag(cst, t, S_END);
    pop_tag(cst);
    return INVALID_REG;
}

/*
 *    for
 *     |-- iterator
 *     |-- gen
 *     |-- block
 */
REG_TYPE do_for(struct CompileState *cst, struct Token* tok)
{
    REG_TYPE reg = do_local(cst, tok->items.head->t, INVALID_REG);
    REG_TYPE itr = do_expression(cst, tok->items.head->next->t, INVALID_REG);
    REG_TYPE i = _do_number(cst, "0", INVALID_REG);

    int t = stack_tag(cst);
    tag(cst, t, S_LOOP);
    tag(cst, t, S_CONTINUE);
    code(cst, OP_ITER, reg, itr, i);
    jump(cst, t, S_END);
    free_tmp(cst, do_expression(cst, tok->items.head->next->next->t, INVALID_REG));
    jump(cst, t, S_LOOP);
    tag(cst, t, S_BREAK);
    tag(cst, t, S_END);
    pop_tag(cst);

    free_tmp(cst, itr);
    free_tmp(cst, i);
    return INVALID_REG;
}

/*
 *    [  items[0] for items[1] in items[2]  ]
 */
REG_TYPE do_comp(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	char buffer[16];
	int l = snprintf(buffer, 16, "comp:%d", get_tag(cst));
    char* name = new_string(cst, l+1);
    struct Token* nt = new_token(cst, t->col, t->row, S_NAME, 0), *nt2, *nt3, *key, *ap;
    memcpy(name, buffer, l+1);
    nt->vn = name;
    r = do_local(cst, nt, INVALID_REG);
    code(cst, OP_LIST,r,0,0);
    key = new_token(cst, t->col, t->row, S_GET, 0);
    nt2 = new_token(cst, t->col, t->row, S_REG, r);
    nt3 = new_token(cst, t->col, t->row, S_SYMBOL, S_NONE);
    append_list_item(cst, &key->items, nt2);
	append_list_item(cst, &key->items, nt3);
    ap = new_token(cst, t->col, t->row, S_SYMBOL, S_EQUAL);
    append_list_item(cst, &ap->items, key);
	append_list_item(cst, &ap->items, t->items.head->t);
	nt = new_token(cst, t->col, t->row, S_FOR, 0);
	append_list_item(cst, &nt->items, t->items.head->next->t);
	append_list_item(cst, &nt->items, t->items.head->next->next->t);
	append_list_item(cst, &nt->items, ap);
    do_expression(cst, nt, INVALID_REG);
    return r;
}

/*
 *     if
 *      |-- elif
 *      |     |-- cond
 *      |     |-- block
 *       ...
 *      |-- else
 *            |-- block
 */
REG_TYPE do_if(struct CompileState *cst, struct Token* tt)
{
    struct TListItem* kk = tt->items.head;
    int t = get_tag(cst);
    int n = 0;
    while (kk)
    {
    	struct Token* tt = kk->t;
        tag(cst, t, MAX_SYMS + n);
        if (tt->type == S_ELIF)
        {
            REG_TYPE a = do_expression(cst, tt->items.head->t, INVALID_REG);
            code(cst, OP_IF, a, 0, 0);
            free_tmp(cst, a);
            jump(cst, t, MAX_SYMS + n + 1);
            free_tmp(cst, do_expression(cst, tt->items.head->next->t, INVALID_REG));
        }
        else if (tt->type == S_ELSE)
            free_tmp(cst, do_expression(cst, tt->items.head->t, INVALID_REG));
        else
            u_error(cst, "huh?", "", tt->col, tt->row);
        jump(cst, t, S_END);
        kk = kk->next;
        n ++;
    }
    tag(cst, t, MAX_SYMS + n);
    tag(cst, t, S_END);
    return INVALID_REG;
}

/*
 *   try
 *    |-- block
 *    |-- except
 *          |-- exc
 *          |-- block
 */
REG_TYPE do_try(struct CompileState *cst, struct Token* tok)
{
    int t = get_tag(cst);
    setjump(cst, t, S_EXCEPT);
    free_tmp(cst, do_expression(cst, tok->items.head->t, INVALID_REG));
    code(cst, OP_SETJMP, 0, 0, 0);
    jump(cst, t, S_END);
    tag(cst, t, S_EXCEPT);
    free_tmp(cst, do_expression(cst, tok->items.head->next->t->items.head->next->t, INVALID_REG));
    tag(cst, t, S_END);
    return INVALID_REG;
}

REG_TYPE do_return(struct CompileState *cst, struct Token* t)
{
	REG_TYPE r;
    if (get_list_len(&t->items)) r = do_expression(cst, t->items.head->t, INVALID_REG);
    else r = _do_none(cst, INVALID_REG);
    code(cst, OP_RETURN, r, 0, 0);
    free_tmp(cst, r);
    return INVALID_REG;
}

REG_TYPE do_raise(struct CompileState *cst, struct Token* t)
{
	REG_TYPE r;
    if (get_list_len(&t->items)) r = do_expression(cst, t->items.head->t, INVALID_REG);
    else r = _do_none(cst, INVALID_REG);
    code(cst, OP_RAISE, r, 0, 0);
    free_tmp(cst, r);
    return INVALID_REG;
}

REG_TYPE do_statements(struct CompileState *cst, struct Token* t)
{
	struct TListItem* kk = t->items.head;
	while (kk)
	{
    	free_tmp(cst, do_expression(cst, kk->t, INVALID_REG));
    	kk = kk->next;
    }
    return INVALID_REG;
}

REG_TYPE do_list(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
    r = get_tmp(cst, r);
    manage_seq(cst, OP_LIST, r, &t->items, 0);
    return r;
}

REG_TYPE do_dict(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
    r = get_tmp(cst, r);
    manage_seq(cst, OP_DICT, r, &t->items, 0);
    return r;
}

REG_TYPE do_get(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
    return infix(cst, OP_GET, t->items.head->t, t->items.head->next->t, r);
}

REG_TYPE do_break(struct CompileState *cst, struct Token* t)
{
	jump(cst, cst->D.tstack->v, S_BREAK);
	return INVALID_REG;
}

REG_TYPE do_continue(struct CompileState *cst, struct Token* t)
{
	jump(cst, cst->D.tstack->v, S_CONTINUE);
	return INVALID_REG;
}

REG_TYPE do_pass(struct CompileState *cst, struct Token* t)
{
	code(cst, OP_PASS, 0, 0, 0);
	return INVALID_REG;
}

static REG_TYPE do_module(struct CompileState *cst, struct Token* t)
{
    do_info(cst, "?");
    free_tmp(cst, do_expression(cst, t->items.head->t, INVALID_REG));
    return INVALID_REG;
}

REG_TYPE do_reg(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	return t->vi;
}

typedef REG_TYPE (*FMAP_FUN) (struct CompileState *cst, struct Token*);
typedef REG_TYPE (*RMAP_FUN) (struct CompileState *cst, struct Token*, REG_TYPE r);

struct ExpMap
{
	FMAP_FUN ff;
	RMAP_FUN rf;
};

struct ExpMap fun_map[] = {
	{0}, //"", 0
	{do_def_wrapper, 0}, //"def", 1
	{do_class, 0}, //"class", 2
	{0}, //"yield", 3
	{do_return, 0}, //"return", 4
	{do_pass, 0}, //"pass", 5
	{0}, //"and", 6
	{0}, //"or", 7
	{0}, //"not", 8
	{0}, //"in", 9
	{do_import, 0}, //"import", 10
	{0}, //"is", 11
	{do_while, 0}, //"while", 12
	{do_break, 0}, //"break", 13
	{do_for, 0}, //"for", 14
	{do_continue, 0}, //"continue", 15
	{do_if, 0}, //"if", 16
	{0}, //"else", 17
	{0}, //"elif", 18
	{do_try, 0}, //"try", 19
	{0}, //"except", 20
	{do_raise, 0}, //"raise", 21
	{0}, //"True", 22
	{0}, //"False", 23
	{0}, //"None", 24
	{do_globals, 0}, //"global", 25
	{do_del, 0}, //"del", 26
	{do_from, 0}, //"from", 27
	{0}, //"-", 28
	{0}, //"+", 29
	{0}, //"*", 30
	{0}, //"**", 31
	{0}, //"/", 32
	{0}, //"%%", 33
	{0}, //"<<", 34
	{0}, //">>", 35
	{0}, //"-=", 36
	{0}, //"+=", 37
	{0}, //"*=", 38
	{0}, //"/=", 39
	{0}, //"=", 40
	{0}, //"==", 41
	{0}, //"!=", 42
	{0}, //"<", 43
	{0}, //">", 44
	{0}, //"|=", 45
	{0}, //"&=", 46
	{0}, //"^=", 47
	{0}, //"<=", 48
	{0}, //">=", 49
	{0}, //"[", 50
	{0}, //"]", 51
	{0}, //"{", 52
	{0}, //"}", 53
	{0}, //"(", 54
	{0}, //")", 55
	{0}, //".", 56
	{0}, //":", 57
	{0}, //",", 58
	{0}, //";", 59
	{0}, //"&", 60
	{0}, //"|", 61
	{0}, //"!", 62
	{0}, //"^", 63
	{0, do_get},  //"get", 64
	{0},  //"nl", 65
	{0},  //"indent", 66
	{0},  //"dedent", 67
	{0, do_symbol},  //"symbol", 68
	{0, do_number},  //"number", 69
	{0, do_name},  //"name", 70
	{0, do_string},  //"string", 71
	{do_statements, 0},  //"statements", 72
	{0, do_list},  //"tuple", 73
	{0},  //"methods", 74
	{0},  //"eof", 75
	{0},  //"():", 76
	{0},  //"notin", 77
	{0},  //"isnot", 78
	{0, do_dict},  //"dict", 79
	{0, do_comp},  //"comp", 80
	{0, do_list},  //"slice", 81
	{0, do_call},  //"call", 82
	{0, do_reg},  //"reg", 83
	{do_module, 0},  //"module", 84
	{0},  //"args", 85
	{0},  //"nargs", 86
	{0},  //"$", 87
	{0},  //"[]", 88
	{0},  //"{}", 89
	{0, do_list},  //"list", 90
	{0},  //"end", 91
	{0},  //"begin", 92
	{0},  //"loop", 93
};

REG_TYPE do_expression(struct CompileState *cst, struct Token* t, REG_TYPE r)
{
	selpos(cst, t->row);
	if (fun_map[t->type].rf)
		return fun_map[t->type].rf(cst, t, r);
	else if (fun_map[t->type].ff)
		return fun_map[t->type].ff(cst, t);
	else
		u_error(cst, "encode", cst->D.code, t->col, t->row);
}

void split(struct CompileState *cst, char* s)
{
	const char* h = s;
	int i = 0, n = 0;
	while (s[i])
	{
		if (s[i] == '\n') n++;
		i ++;
	}
	n++;
	cst->D.lines = (const char**)malloc(n * sizeof(const char*));
	cst->D.linenum = n;
	i = 0;
	n = 0;
	while (s[i])
	{
		if (s[i] == '\n')
		{
			cst->D.lines[n] = h;
			s[i] = '\0';
			h = &s[i + 1];
			n++;
		}
		i++;
	}
	cst->D.lines[n] = h;
}

const char* encode(struct CompileState *cst, const char* fname, char* s, struct Token* t)
{
	struct Token* nt = new_token(cst, 1, 1, S_MODULE, S_MODULE);
	append_list_item(cst, &nt->items, t);
	memset(&cst->D, 0, sizeof(struct DState));
    cst->D.fname = fname;
	cst->D.code = s;
	split(cst, s);
    begin(cst, true);
    do_expression(cst, nt, INVALID_REG);
    end(cst);
    merge_tags(cst);
    return cst->D.so;
}

const char* compile(const char* fname, char* code, int* size, int *res)
{
	struct TList tokens;
	struct Token* t;
	const char* r;
	struct CompileState c;

	memset(&c, 0, sizeof(struct CompileState));

	if (setjmp(c.error_jmp_buf))
	{
		*res = 0;
		r = strdup(get_compile_error(&c));
		clear_compile_mem(&c);
		return r;
	}

	tokens = tokenize(&c, code);
	t = parse(&c, code, tokens);
	encode(&c, fname, code, t);
	r = (const char *)malloc(c.D.nso);
	memcpy(r, c.D.so, c.D.nso);
	*size = c.D.nso;
	*res = 1;
	clear_compile_mem(&c);
	return r;
}

