#pragma once

const char* get_compile_error(struct CompileState *c);
void clear_compile_mem(struct CompileState *c);

struct Token* new_token(struct CompileState *c, int col, int row, int type, int vs);
void append_list_item(struct CompileState *cst, struct TList*list, struct Token* t);
void move_list_items(struct TList*dstlist, struct TList*srclist, int st);
int get_list_len(struct TList* list);
void append_intList_item(struct CompileState *c, struct IntList*list, int v);
struct IntListItem* new_intList_item(struct CompileState *c, int v);
struct IntListItem* find_int_item(struct IntList* lst, int v);
char *new_string(struct CompileState *c, int size);
void append_strList_item(struct CompileState *c, struct StrList*list, const char *s);
struct StrListItem* find_str_item(struct StrList* lst, char* s);
void u_error(struct CompileState *c, const char* ctx, const char*s, int col, int row);
struct Item* new_code_item(struct CompileState *c, int isdata);
void append_itemList_item(struct ItemList*list, struct Item* item);
void remove_itemList_item(struct ItemList*list, struct Item* item);