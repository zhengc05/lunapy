#include "sym.h"
#include "tokenize.h"
#include "mem.h"

const char*ISYMBOLS = "`-=[];,./~!@$%%^&*()+{}:<>?|";
int B_BEGIN[] = {S_LEFTQUARE, S_LEFTCIRCLE, S_LEFTBIG};
int B_END[] = {S_RIGHTQUARE, S_RIGHTCIRCLE, S_RIGHTBIG};

bool in_str(const char *s, char c)
{
	int i = 0;
	while (s[i])
		if (s[i++] == c) return true;
	return false;
}

bool in_sets(int sets[], int size, int s)
{
	for (int i = 0; i < size; i++)
	{
		if (sets[i] == s) return true;
	}
	return false;
}

char* new_str(struct CompileState *c, const char *s, int f, int n)
{
	char* str = new_string(c, n + 1);
	for (int j = 0; j < n; j ++)
	{
		str[j] = s[f+j];
	}
	str[n] = '\0';
	return str;
}

static void clean(char *s)
{
	int i = 0;
	while (s[i])
	{
		if (s[i] == '\r')
		{
			if (s[i+1] && s[i+1]=='\n')
				s[i] = ' ';
			else
				s[i] = '\n';
		}
		i++;
	}
}

struct Token* add_token(struct CompileState *c, int type, int vs)
{
	struct Token* t = new_token(c, c->T.col, c->T.row, type, vs);
	append_list_item(c, &c->T.res, t);
	return t;
}

static int do_nl(struct CompileState *c, char *s, int i, int size)
{
	if (!c->T.braces)
	{
		add_token(c, S_NL, 0);
	}
	i++;
	c->T.nl = true;
	c->T.y++;
	c->T.yi = i;
	return i;
}

static void indent(struct CompileState *c, int v)
{
	if (v == c->T.indents->v);
	else if (v > c->T.indents->v)
	{
		struct IntListItem *p = new_intList_item(c, v);
		p->next = c->T.indents;
		c->T.indents = p;
		add_token(c, S_INDENT, v);
	}
	else if (v < c->T.indents->v)
	{
		struct IntListItem *p = c->T.indents;
		while (p)
		{
			if (p->v == v) break;
			add_token(c, S_DEDENT, p->v);
			p = p->next;
		}
		c->T.indents = p;
	}
}

static int do_indent(struct CompileState *cst, char *s, int i, int size)
{
	char c = 0;
	int v = 0;
	while (i < size)
	{
		c = s[i];
		if (c != ' ' && c != '\t') break;
		i ++;
		v ++;
	}
	if (c != '\n' && c != '#' && !cst->T.braces)
		indent(cst, v);
	return i; 
}

static int do_symbol(struct CompileState *cst, char *s, int i, int size)
{
	int f = i, n;
	int sym = in_symbol(&s[f], i - f + 1);
	int m = -1;
	while (i < size)
	{
		char c = s[i];
		if (!in_str(ISYMBOLS, c)) break;
		i ++;
		m = in_symbol(&s[f], i - f + 1);
		if (m) sym = m;
	}
	n = strlen(CONSTR[sym]);
	i = f + n;
	add_token(cst, S_SYMBOL, sym);
	if (in_sets(B_BEGIN, sizeof(B_BEGIN) / sizeof(int), sym)) cst->T.braces ++;
	if (in_sets(B_END, sizeof(B_BEGIN) / sizeof(int), sym)) cst->T.braces --;
	return i;
}

static int do_number(struct CompileState *cst, char *s, int i, int size)
{
	char c = '\0';
	int f = i, n;
	i ++;
	while (i < size)
	{
		c = s[i];
		if ((c < '0' || c > '9') && (c < 'a' || c > 'f') && c != 'x') break;
		i ++;
	}
	if (c == '.')
	{
		i ++;
		while (i < size)
		{
			c = s[i];
			if (c < '0' || c > '9') break;
			i ++;
		}
	}
	n = i - f;
	add_token(cst, S_NUMBER, 0)->vn = new_str(cst, s, f, n);
	return i;
}

static int do_name(struct CompileState *cst, char *s, int i, int size)
{
	int sym;
	char c;
	int f = i, n;
	while (i < size)
	{
		c = s[i];
		if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_') break;
		i ++;
	}
	n = i - f;
	sym = in_symbol(&s[f], n);
	if (sym) add_token(cst, S_SYMBOL, sym);
	else{
		add_token(cst, S_NAME, 0)->vn = new_str(cst, s, f, n);
	}
	return i;
}

static int do_string(struct CompileState *cst, char *s, int i, int size)
{
	int f = i;
	char q = s[i], c;
	i++;
	if ((size - i) >= 5 && s[i] == q && s[i+1] == q) // """
	{
		i += 2;
		while (i < size - 2)
		{
			c = s[i];
			if (c == q && s[i+1] == q && s[i+2] == q)
			{
				i += 3;
				add_token(cst, S_STRING, 0)->vn = new_str(cst, s, f + 3, i - f - 4);
				break;
			}
			else
			{
				i ++;
				if (c == '\n')
				{
					cst->T.y ++;
					cst->T.yi = i;
				}
			}
		}
	}
	else
	{
		char* b;
		int n = i;
		while (i < size)
		{
			c = s[i];
			if (c == '\\')
			{
				i ++;
				c = s[i];
				if (c == 'n') c = '\n';
				if (c == 'r') c = 13;
				if (c == 't') c = '\t';
				if (c == '0') c = '\0';
				i ++;
			}
			else if (c == q)
			{
				i ++;
				b = new_str(cst, s, f + 1, i - f - 2);
				break;
			}
			else
				i ++;
		}

		i = n;
		n = 0;
		while (i < size)
		{
			c = s[i];
			if (c == '\\')
			{
				i++;
				c = s[i];
				if (c == 'n') c = '\n';
				if (c == 'r') c = 13;
				if (c == 't') c = '\t';
				if (c == '0') c = '\0';
				b[n++] = c;
				i++;
			}
			else if (c == q)
			{
				i++;
				b[n++] = '\0';
				add_token(cst, S_STRING, 0)->vn = b;
				break;
			}
			else
			{
				i++;
				b[n++] = c;
			}
		}
	}
	return i;
}

static int do_comment(char *s, int i, int size)
{
	i ++;
	while (i < size)
	{
		char c = s[i];
		if (c == '\n') break;
		i ++;
	}
	return i;
}

static void do_tokenize(struct CompileState *cst, char *s, int i, int size)
{
	while (i < size)
	{
		char c = s[i];
		cst->T.row = cst->T.y;
		cst->T.col = i - cst->T.yi + 1;
		if (cst->T.nl)
		{
			cst->T.nl = false;
			i = do_indent(cst, s, i, size);
		}else{
			switch (c)
			{
			case '\n':
				i = do_nl(cst, s, i, size);
				break;
			case '\"':
			case '\'':
				i = do_string(cst, s, i, size);
				break;
			case '#':
				i = do_comment(s, i, size);
				break;
			case ' ':
			case '\t':
				i ++;
				break;
			default:
				if (in_str(ISYMBOLS, c))
					i = do_symbol(cst, s, i, size);
				else if (c >= '0' && c <= '9')
					i = do_number(cst, s, i, size);
				else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
					i = do_name(cst, s, i, size);
				else if (c == '\\' && s[i+1] == '\n')
				{
					i += 2;
					cst->T.y ++;
					cst->T.yi = i;
				}
				else
					u_error(cst, "tokenize", s, cst->T.col, cst->T.row);
			}
		}

	}
	indent(cst, 0);
}

struct TList tokenize(struct CompileState *c, char* input_str)
{
	clean(input_str);
	memset(&c->T, 0, sizeof(struct TData));
	c->T.y = 1;
	c->T.nl = true;
	c->T.indents = new_intList_item(c, 0);
	do_tokenize(c, input_str, 0, strlen(input_str));
	return c->T.res;
}