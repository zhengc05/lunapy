#pragma once

#include <setjmp.h>
#include <stdbool.h>
#include <string.h>
#include "mem.h"

struct Token;
struct TList;

struct CompileState;

typedef struct Token* (*NUD_FUN) (struct CompileState *c, struct Token*);
typedef struct Token* (*LED_FUN) (struct CompileState *c, struct Token*, struct Token*);

struct MapEntity
{
	int lbp;
	int bp;
	NUD_FUN nud;
	LED_FUN led;
	int type;
	int val;
};

struct TListItem
{
	struct Token* t;
	struct TListItem* next;
};

struct TList
{
	struct TListItem* head;
	int num;
	struct TListItem* tail;
};

struct Token
{
	int col;
	int row;
	int type;
	union{
		int vi;
		int vs;
		char *vn;
	};
	int lbp;
	int bp;
	NUD_FUN nud;
	LED_FUN led;
	struct TList items;
};

enum ItemType
{
	I_CODE = 0,
	I_DATA,
	I_TAG,
	I_JUMP,
	I_SETJMP,
	I_FNC,
	I_REGS
};

struct Item
{
	enum ItemType isdata;
	union {
		unsigned char d[4];
		float f;
		int i;
	};
	int tag_id;
	struct Item* next;
};

struct IntListItem
{
	int v;
	struct IntListItem *next;
};

struct IntList
{
	struct IntListItem *head;
	int num;
	struct IntListItem *tail;
};

struct StrListItem
{
	const char* s;
	struct StrListItem* next;
};

struct StrList
{
	struct StrListItem *head;
	int num;
	struct StrListItem* tail;
};

struct ItemList
{
	struct Item *head;
	int num;
	struct Item* tail;
};

struct TokenPool
{
	struct  Token elem[128];
	int null_idx;
	struct  TokenPool* next;
};

struct ListPool
{
	struct TListItem elem[128];
	int null_idx;
	struct ListPool* next;
};

struct IntListPool
{
	struct IntListItem elem[128];
	int null_idx;
	struct IntListPool* next;
};

struct StringPool
{
	int null_idx;
	int total_size;
	struct StringPool* next;
	char mem[];
};

struct StrListPool
{
	struct StrListItem elem[128];
	int null_idx;
	struct StrListPool* next;
};

struct ItemPool
{
	struct Item elem[128];
	int null_idx;
	struct ItemPool* next;
};

struct TData
{
	int y;
	int yi;
	int col;
	int row;
	int braces;
	bool nl;
	struct TList res;
	struct IntListItem *indents;
};

struct StackItem
{
	int k;
	struct MapEntity v;
	struct StackItem* next;
};

struct PData
{
	const char *s;
	struct Token* token;
	struct TList tokens;
	struct TListItem* pos;
	struct StackItem *stack;
	int _terminal;
};

struct Scope
{
	struct StrList vars;
	char* r2n[256];
	int _tmpi;
	int mreg;
	int snum;
	bool _globals;
	int lineno;
	struct StrList globals;
	struct StrList rglobals;
	struct Item* cregs;
	int _scopei;
	int tmpc;
	struct Scope* next;
};

struct DState
{
	const char* fname;
	const char* code;
	const char** lines;
	int linenum;
	struct ItemList out;
	char* so;
	int nso;
	int _tagi;
	struct IntListItem* tstack;
	struct Scope* scope;
};

struct CompileState
{
	struct TokenPool *token_pool;
	struct ListPool *list_pool;
	struct IntListPool *intList_pool;
	struct StringPool *string_pool;
	struct StrListPool *strList_pool;
	struct ItemPool *item_pool;
	jmp_buf error_jmp_buf;
	char *error_buffer;

	struct TData T;

	struct PData P;
	struct MapEntity *dmap;
	struct MapEntity *gmap;
	struct MapEntity *omap;

	struct DState D;
};

bool in_sets(int sets[], int size, int s);
void u_error(struct CompileState *c, const char* ctx, const char*s, int col, int row);

struct TList tokenize(struct CompileState *c, char* input_str);
struct Token* parse(struct CompileState *c, const char* s, struct TList tokens);
const char* encode(struct CompileState *c, const char* fname, char* s, struct Token* t);

