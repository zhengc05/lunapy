#include "sym.h"
#include "tokenize.h"
#include <stdarg.h>
#include <stdlib.h>



struct Token* itself(struct CompileState *cst, struct Token* t)
{
	return t;
}

struct MapEntity null_ent = { 0, 0, itself, 0, 0, 0 };

bool check(struct Token* t, int n, ...)
{
	int val;
	bool r = false;
	va_list vl;
	va_start(vl,n);
	for(int i = 0; i < n; i ++)
	{
		val = va_arg(vl, int);
		if (val == 0)
		{
			r = true;
			break;
		}
		else if (t->type == val)
		{
			r = true;
			break;
		}else if(t->type == S_SYMBOL && t->vs == val)
		{
			r = true;
			break;
		}
	}
	va_end(vl);
	return r;
}

void tweak(struct CompileState *c, int k, bool v)
{
	struct StackItem *p = (struct StackItem *)malloc(sizeof(struct StackItem));
	p->k = k;
	p->v = c->dmap[k];
	p->next = c->P.stack;
	c->P.stack = p;
	if (v) c->dmap[k] = c->omap[k];
	else c->dmap[k] = null_ent;
}

void restore(struct CompileState *c)
{
	struct StackItem *p = c->P.stack;
	c->P.stack = p->next;
	c->dmap[p->k] = p->v;
	free(p);
}

struct Token* do_token(struct CompileState *c, struct Token* t)
{
	if (t->type == S_SYMBOL)
	{
		t->lbp = c->dmap[t->vs].lbp;
		t->bp = c->dmap[t->vs].bp;
		t->nud = c->dmap[t->vs].nud;
		t->led = c->dmap[t->vs].led;
		if (c->dmap[t->vs].type) t->type = c->dmap[t->vs].type;
		if (c->dmap[t->vs].val) t->vs = c->dmap[t->vs].val;
	}else{
		t->lbp = c->dmap[t->type].lbp;
		t->bp = c->dmap[t->type].bp;
		t->nud = c->dmap[t->type].nud;
		t->led = c->dmap[t->type].led;
		if (c->dmap[t->type].val) t->vs = c->dmap[t->type].val;
		if (c->dmap[t->type].type) t->type = c->dmap[t->type].type;
	}

	return t;
}

void advance(struct CompileState *c, int v)
{
	struct Token* t;
	if (!check(c->P.token, 1, v))
	{
		u_error(c, "expected ", "", c->P.token->col, c->P.token->row);
	}
	
	if (c->P.pos)
	{
		t = c->P.pos->t;
		c->P.pos = c->P.pos->next;
	}
	else
	{
		t = new_token(c, 0, 0, S_EOF, S_EOF);
	}
	c->P.token = do_token(c, t);

	c->P._terminal++;
	if (check(c->P.token, 4, S_NL, S_EOF, S_SEMICOLON, S_DEDENT))
		c->P._terminal = 0;
}

void terminal(struct CompileState *c)
{
	if (c->P._terminal > 1)
		u_error(c, "invalid statement", "", c->P.token->col, c->P.token->row);
}

struct Token* expression(struct CompileState *c, int rbp)
{
	struct Token* t = c->P.token;
	struct Token* left;
	advance(c, 0);
	left = t->nud(c, t);
	while (rbp < c->P.token->lbp)
	{
		t = c->P.token;
		advance(c, 0);
		if (!t->led) u_error(c, "parse", "", t->col, t->row);
		left = t->led(c, t,left);
	}
	return left;
}

struct Token* infix_led(struct CompileState *c, struct Token* t, struct Token* left)
{
	append_list_item(c, &t->items, left);
	append_list_item(c, &t->items, expression(c, t->bp));
	return t;
}

struct Token* infix_is(struct CompileState *c, struct Token* t, struct Token* left)
{
	if (check(c->P.token, 1, S_NOT))
	{
		t->vs = S_ISNOT;
		advance(c, S_NOT);
	}
	append_list_item(c, &t->items, left);
	append_list_item(c, &t->items, expression(c, t->bp));
	return t;
}

struct Token* infix_not(struct CompileState *c, struct Token* t, struct Token* left)
{
	advance(c, S_IN);
	t->vs = S_NOTIN;
	append_list_item(c, &t->items, left);
	append_list_item(c, &t->items, expression(c, t->bp));
	return t;
}

struct Token* infix_tuple(struct CompileState *c, struct Token* t, struct Token* left)
{
    struct Token* r = expression(c, t->bp);
    if (left->vs == S_COMMA)
    {
        append_list_item(c, &left->items, r);
        return left;
    }
	append_list_item(c, &t->items, left);
	append_list_item(c, &t->items, r);
    t->type = S_TUPLE;
    return t;
}

struct TList lst(struct CompileState *c, struct Token* t)
{
	struct TList items = { 0 };

	if (!t) return items;
	if (check(t, 3, S_COMMA, S_TUPLE, S_STATEMENTS))
		return t->items;
	append_list_item(c, &items, t);
	return items;
}

struct Token* ilst(struct CompileState *c, int type, struct Token* tok)
{
	struct Token* nt = new_token(c, tok->col, tok->row, type, type);
	nt->items = lst(c, tok);
	return nt;
}

struct Token* call_led(struct CompileState *c, struct Token* t, struct Token* left)
{
    struct Token* r = new_token(c, t->col, t->row, S_CALL, S_DOLLAR);
	append_list_item(c,&r->items, left);
    while (!check(c->P.token, 1, S_RIGHTCIRCLE))
    {
        tweak(c, S_COMMA, 0);
        append_list_item(c,&r->items, expression(c, 0));
        if (c->P.token->vs == S_COMMA) advance(c, S_COMMA);
        restore(c);
	}
    advance(c, S_RIGHTCIRCLE);
    return r;
}

struct Token* get_led(struct CompileState *c, struct Token* t, struct Token* left)
{
    struct Token* r = new_token(c, t->col, t->row, S_GET, S_DOT), *nt;
    bool more = false;
	append_list_item(c,&r->items, left);
    while (!check(c->P.token, 1, S_RIGHTQUARE))
    {
        more = false;
        if (check(c->P.token, 1, S_COLON))
        	append_list_item(c,&r->items, new_token(c, c->P.token->col, c->P.token->row, S_SYMBOL, S_NONE));
        else
            append_list_item(c,&r->items, expression(c, 0));
        if (check(c->P.token, 1, S_COLON))
        {
        	advance(c, S_COLON);
            more = true;
        }
    }
    if (more)
        append_list_item(c,&r->items, new_token(c, c->P.token->col, c->P.token->row, S_SYMBOL, S_NONE));
    if (get_list_len(&r->items) > 2)
    {
    	nt = new_token(c, t->col, t->row, S_SLICE, S_COLON);
		move_list_items(&nt->items, &r->items, 1);
    	r->items.head->t = left;
		append_list_item(c,&r->items, nt);
    }
    advance(c, S_RIGHTQUARE);
    return r;
}

struct Token* dot_led(struct CompileState *c, struct Token* t, struct Token* left)
{
    struct Token* r = expression(c, t->bp);
    r->type = S_STRING;
	append_list_item(c,&t->items, left);
	append_list_item(c,&t->items, r);
    return t;
}

struct Token* paren_nud(struct CompileState *c, struct Token* t)
{
	struct Token* r;
    tweak(c, S_COMMA, 1);
    r = expression(c, 0);
    restore(c);
    advance(c, S_RIGHTCIRCLE);
    return r;
}

struct Token* list_nud(struct CompileState *c, struct Token* t)
{
	struct TList items = { 0 };

    t->type = S_LIST;
    t->vs = S_EMPTYQUAD;
    t->items = items;
    tweak(c, S_COMMA, 0);
    while (!check(c->P.token, 2, S_FOR, S_RIGHTQUARE))
    {
        append_list_item(c,&t->items, expression(c, 0));
        if (c->P.token->vs == S_COMMA) advance(c, S_COMMA);
    }
    if (check(c->P.token, 1, S_FOR))
    {
        t->type = S_COMP;
        advance(c, S_FOR);
        tweak(c, S_IN, 0);
        append_list_item(c,&t->items, expression(c, 0));
        advance(c, S_IN);
        append_list_item(c,&t->items, expression(c, 0));
        restore(c);
    }
    restore(c);
    advance(c, S_RIGHTQUARE);
    return t;
}

struct Token* dict_nud(struct CompileState *c, struct Token* t)
{
	struct TList items = { 0 };

    t->type = S_DICT;
    t->vs = S_EMPTYBIG;
    t->items = items;
    tweak(c, S_COMMA, 0);
    while (!check(c->P.token, 1, S_RIGHTBIG))
    {
    	append_list_item(c,&t->items, expression(c, 0));
        if (check(c->P.token, 2, S_COLON, S_COMMA)) advance(c, 0);
    }
    restore(c);
    advance(c, S_RIGHTBIG);
    return t;
}

void iblock(struct CompileState *c, struct TList* items)
{
	while (check(c->P.token, 2, S_NL, S_SEMICOLON)) advance(c, 0);
	while (1)
	{
		append_list_item(c, items, expression(c, 0));
		terminal(c);
		while (check(c->P.token, 2, S_NL, S_SEMICOLON)) advance(c, 0);
		if (check(c->P.token, 2, S_DEDENT, S_EOF)) break;
	}
}

struct Token* block(struct CompileState *c)
{
	struct TList items = {0};
	struct Token* tok = c->P.token;
	
	if (check(c->P.token, 1, S_NL))
	{
		while (check(c->P.token, 1, S_NL)) advance(c, 0);
		advance(c, S_INDENT);
		iblock(c, &items);
		advance(c, S_DEDENT);
	}
	else
	{
		append_list_item(c,&items, expression(c, 0));
		while (check(c->P.token, 1, S_SEMICOLON))
		{
			advance(c, S_SEMICOLON);
			append_list_item(c,&items, expression(c, 0));
		}
		terminal(c);
	}
	while (check(c->P.token, 1, S_NL)) advance(c, 0);

	if (get_list_len(&items) > 1)
	{
		struct Token *r = new_token(c, tok->col, tok->row, S_STATEMENTS, S_SEMICOLON);
		r->items = items;
		return r;
	}
	return items.head->t;
}

struct Token* def_nud(struct CompileState *c, struct Token* t)
{
	struct Token* tmp;
	append_list_item(c,&t->items, c->P.token);
	advance(c, 0);
	advance(c, S_LEFTCIRCLE);
	tmp = new_token(c, t->col, t->row, S_SYMBOL, S_EMPTYDEF);
	append_list_item(c,&t->items, tmp);
	while (!check(c->P.token, 1, S_RIGHTCIRCLE))
	{
		tweak(c, S_COMMA, false);
		append_list_item(c,&tmp->items, expression(c, 0));
		if (check(c->P.token, 1, S_COMMA))
			advance(c, S_COMMA);
		restore(c);
	}
	advance(c, S_RIGHTCIRCLE);
	advance(c, S_COLON);
	append_list_item(c,&t->items, block(c));
	return t;
}

struct Token* while_nud(struct CompileState *c, struct Token* t)
{
	append_list_item(c,&t->items, expression(c, 0));
	advance(c, S_COLON);
	append_list_item(c,&t->items, block(c));
	return t;
}

struct Token* class_nud(struct CompileState *c, struct Token* t)
{
	append_list_item(c,&t->items, expression(c, 0));
	advance(c, S_COLON);
	append_list_item(c,&t->items, ilst(c, S_METHODS, block(c)));
	return t;
}

struct Token* from_nud(struct CompileState *c, struct Token* t)
{
    append_list_item(c,&t->items, expression(c, 0));
    advance(c, S_IMPORT);
    append_list_item(c,&t->items, expression(c, 0));
    return t;
}

struct Token* for_nud(struct CompileState *c, struct Token* t)
{
	tweak(c, S_IN, false);
	append_list_item(c,&t->items, expression(c, 0));
	advance(c, S_IN);
	append_list_item(c,&t->items, expression(c, 0));
	restore(c);
	advance(c, S_COLON);
	append_list_item(c,&t->items, block(c));
	return t;
}

struct Token* if_nud(struct CompileState *c, struct Token* t)
{
	struct Token* a, *b, *tok, *nt;
	a = expression(c, 0);
	advance(c, S_COLON);
	b = block(c);
	nt = new_token(c, t->col, t->row, S_ELIF, S_ELIF);
	append_list_item(c,&nt->items, a);
	append_list_item(c,&nt->items, b);
	append_list_item(c,&t->items, nt);
	while (check(c->P.token, 1, S_ELIF))
	{
		tok = c->P.token;
		advance(c, S_ELIF);
		a = expression(c, 0);
		advance(c, S_COLON);
		b = block(c);
		nt = new_token(c, tok->col, tok->row, S_ELIF, S_ELIF);
		append_list_item(c,&nt->items, a);
		append_list_item(c,&nt->items, b);
		append_list_item(c,&t->items, nt);
	}
	if (check(c->P.token, 1, S_ELSE))
	{
		tok = c->P.token;
		advance(c, S_ELSE);
		advance(c, S_COLON);
		b = block(c);
		nt = new_token(c, tok->col, tok->row, S_ELSE, S_ELSE);
		append_list_item(c,&nt->items, b);
		append_list_item(c,&t->items, nt);
	}
	return t;
}

struct Token* try_nud(struct CompileState *c, struct Token* t)
{
	struct Token* tok, *a, *b, *nt;
    advance(c, S_COLON);
    b = block(c);
	append_list_item(c,&t->items, b);
    while (check(c->P.token, 1, S_EXCEPT))
    {
        tok = c->P.token;
        advance(c, S_EXCEPT);
        if (!check(c->P.token, 1, S_COLON))
        {
        	a = expression(c, 0);
        }
        else
        {
        	a = new_token(c, tok->col, tok->row, S_SYMBOL, S_NONE);
        }
        advance(c, S_COLON);
        b = block(c);
        nt = new_token(c, tok->col, tok->row, S_EXCEPT, S_EXCEPT);
		append_list_item(c,&nt->items, a);
		append_list_item(c,&nt->items, b);
		append_list_item(c,&t->items, nt);
    }
    return t;
}

struct Token*  prefix_nud(struct CompileState *c, struct Token* t)
{
	int bp = t->bp;
	append_list_item(c,&t->items, expression(c, bp));
	return t;
}

struct Token* prefix_nud0(struct CompileState *c, struct Token* t)
{
	if (check(c->P.token, 4, S_NL, S_SEMICOLON, S_EOF, S_DEDENT)) return t;
	return prefix_nud(c, t);
}

struct Token* prefix_nuds(struct CompileState *c, struct Token* t)
{
	struct Token* r = expression(c, 0);
	return ilst(c, t->type,r);
}

struct Token* prefix_neg(struct CompileState *c, struct Token* t)
{
    struct Token* r = expression(c, 50), *nt;
    if (r->type == S_NUMBER)
    {
    	if (r->vn[0] == '-')
    		memmove(r->vn, &r->vn[1], strlen(r->vn)+1);
    	else
    	{
    		const char* vm = r->vn;
    		int l = strlen(r->vn)+1;
    		r->vn = new_string(c, l+1);
    		r->vn[0] = '-';
    		memcpy(&r->vn[1], vm, l);
    	}
        return r;
    }
    nt = new_token(c, t->col, t->row, S_NUMBER, 0);
    nt->vn = new_string(c, 2);
    nt->vn[0] = '0';
    nt->vn[1] = '\0';
	append_list_item(c,&t->items, nt);
	append_list_item(c,&t->items, r);
    return t;
}

struct Token* vargs_nud(struct CompileState *c, struct Token* t)
{
    struct Token* r = prefix_nud(c, t);
    t->type = S_ARGS;
    t->vs = S_STAR;
    return t;
}

struct Token* nargs_nud(struct CompileState *c, struct Token* t)
{
    struct Token* r = prefix_nud(c, t);
    t->type = S_NARGS;
    t->vs = S_STARTWO;
    return t;
}

struct MapEntity base_dmap[] = {
	{0}, //""
	{0, 0, def_nud, 0, S_DEF, 0}, //"def", 1
	{0, 0, class_nud, 0, S_CLASS, 0}, //"class", 2
	{0}, //"yield", 3
	{0, 10, prefix_nud0, 0, S_RETURN, 0}, //"return", 4
	{0, 0, itself, 0, S_PASS, 0}, //"pass", 5
	{32, 32, 0, infix_led, 0, 0}, //"and", 6
	{30, 30, 0, infix_led, 0, 0}, //"or", 7
	{35, 35, prefix_nud, infix_not, 0, 0}, //"not", 8
	{40, 40, 0, infix_is, 0, 0}, //"in", 9
	{0, 20, prefix_nuds, 0, S_IMPORT, 0}, //"import", 10
	{40, 40, 0, infix_is, 0, 0}, //"is", 11
	{0, 0, while_nud, 0, S_WHILE, 0}, //"while", 12
	{0, 0, itself, 0, S_BREAK, 0}, //"break", 13
	{0, 0, for_nud, 0, S_FOR, 0}, //"for", 14
	{0, 0, itself, 0, S_CONTINUE, 0}, //"continue", 15
	{0, 0, if_nud, 0, S_IF, 0}, //"if", 16
	{0, 0, itself, 0, 0, 0}, //"else", 17
	{0, 0, itself, 0, 0, 0}, //"elif", 18
	{0, 0, try_nud, 0, S_TRY, 0}, //"try", 19
	{0, 0, itself, 0, 0, 0}, //"except", 20
	{0, 20, prefix_nud0, 0, S_RAISE, 0}, //"raise", 21
	{0, 0, itself, 0, 0, 0}, //"True", 22
	{0, 0, itself, 0, 0, 0}, //"False", 23
	{0, 0, itself, 0, 0, 0}, //"None", 24
	{0, 20, prefix_nuds, 0, S_GLOBAL, 0}, //"global", 25
	{0, 10, prefix_nuds, 0, S_DEL, 0}, //"del", 26
	{0, 20, from_nud, 0, S_FROM, 0}, //"from", 27
	{50, 50, prefix_neg, infix_led, 0, 0}, //"-", 28
	{50, 50, 0, infix_led, 0, 0}, //"+", 29
	{60, 60, vargs_nud, infix_led, 0, 0}, //"*", 30
	{65, 65, nargs_nud, infix_led, 0, 0}, //"**", 31
	{60, 60, 0, infix_led, 0, 0}, //"/", 32
	{60, 60, 0, infix_led, 0, 0}, //"%%", 33
	{36, 36, 0, infix_led, 0, 0}, //"<<", 34
	{36, 36, 0, infix_led, 0, 0}, //">>", 35
	{10, 10, 0, infix_led, 0, 0}, //"-=", 36
	{10, 10, 0, infix_led, 0, 0}, //"+=", 37
	{10, 10, 0, infix_led, 0, 0}, //"*=", 38
	{10, 10, 0, infix_led, 0, 0}, //"/=", 39
	{10, 9, 0, infix_led, 0, 0}, //"=", 40
	{40, 40, 0, infix_led, 0, 0}, //"==", 41
	{40, 40, 0, infix_led, 0, 0}, //"!=", 42
	{40, 40, 0, infix_led, 0, 0}, //"<", 43
	{40, 40, 0, infix_led, 0, 0}, //">", 44
	{10, 10, 0, infix_led, 0, 0}, //"|=", 45
	{10, 10, 0, infix_led, 0, 0}, //"&=", 46
	{10, 10, 0, infix_led, 0, 0}, //"^=", 47
	{40, 40, 0, infix_led, 0, 0}, //"<=", 48
	{40, 40, 0, infix_led, 0, 0}, //">=", 49
	{70, 80, list_nud, get_led, 0, 0}, //"[", 50
	{0, 0, itself, 0, 0, 0}, //"]", 51
	{0, 0, dict_nud, 0, 0, 0}, //"{", 52
	{0, 0, itself, 0, 0, 0}, //"}", 53
	{70, 80, paren_nud, call_led, 0, 0}, //"(", 54
	{0, 0, itself, 0, 0, 0}, //")", 55
	{80, 80, 0, dot_led, S_GET, 0}, //".", 56
	{0, 0, itself, 0, 0, 0}, //":", 57
	{20, 20, 0, infix_tuple, 0, 0}, //",", 58
	{0, 0, itself, 0, 0, 0}, //";", 59
	{32, 32, 0, infix_led, 0, 0}, //"&", 60
	{30, 30, 0, infix_led, 0, 0}, //"|", 61
	{0}, //"!", 62
	{31, 31, 0, infix_led, 0, 0}, //"^", 63
	{0}, //"get", 64
	{0, 0, itself, 0, 0, S_NL}, //"nl", 65
	{0, 0, itself, 0, 0, 0}, //"indent", 66
	{0, 0, itself, 0, 0, 0}, //"dedent", 67
	{0}, //"symbol", 68
	{0, 0, itself, 0, 0, 0}, //"number", 69
	{0, 0, itself, 0, 0, 0}, //"name", 70
	{0, 0, itself, 0, 0, 0}, //"string", 71
	{0}, //"statements", 72
	{0}, //"tuple", 73
	{0}, //"methods", 74
	{0, 0, 0, 0, S_EOF, S_EOF}, //"eof", 75
};

void init_parse(struct CompileState *c)
{
	c->P._terminal = 0;
	c->dmap = (struct MapEntity *)malloc(sizeof(base_dmap));
	memcpy(c->dmap, base_dmap, sizeof(base_dmap));
	c->gmap = (struct MapEntity *)malloc(sizeof(base_dmap));
	memcpy(c->gmap, base_dmap, sizeof(base_dmap));
	c->omap = (struct MapEntity *)malloc(sizeof(base_dmap));
	memcpy(c->omap, base_dmap, sizeof(base_dmap));
	c->P.pos = c->P.tokens.head;
	advance(c, 0);
}

static struct Token* do_module(struct CompileState *c)
{
	struct Token* tok = c->P.token;
	struct TList items = { 0 };
	iblock(c, &items);
	advance(c, S_EOF);
	if (get_list_len(&items) > 1)
	{
		struct Token* r = new_token(c, tok->col, tok->row, S_STATEMENTS, S_SEMICOLON);
		r->items = items;
		return r;
	}
	return items.head->t;
}

struct Token* parse(struct CompileState *c, const char* s, struct TList tokens)
{
	struct Token* r;
	//s = tokenize.clean(s);
	memset(&c->P, 0, sizeof(struct PData));
	c->P.s = s;
	c->P.tokens = tokens;
	init_parse(c);
	r = do_module(c);
	return r;
}