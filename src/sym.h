#pragma once

#define S_DEF 1
#define S_CLASS 2
#define S_YIELD 3
#define S_RETURN 4
#define S_PASS 5
#define S_AND 6
#define S_OR 7
#define S_NOT 8
#define S_IN 9
#define S_IMPORT 10
#define S_IS 11
#define S_WHILE 12
#define S_BREAK 13
#define S_FOR 14
#define S_CONTINUE 15
#define S_IF 16
#define S_ELSE 17
#define S_ELIF 18
#define S_TRY 19
#define S_EXCEPT 20
#define S_RAISE 21
#define S_TRUE 22
#define S_FALSE 23
#define S_NONE 24
#define S_GLOBAL 25
#define S_DEL 26
#define S_FROM 27
#define S_MINUS 28
#define S_PLUS 29
#define S_STAR 30
#define S_STARTWO 31
#define S_DIV 32
#define S_CENT 33
#define S_LOWTWO 34
#define S_BIGTWO 35
#define S_MINUSEQUAL 36
#define S_PLUSEQUAL 37
#define S_STAREQUAL 38
#define S_DIVEQUAL 39
#define S_EQUAL 40
#define S_EQUALTWO 41
#define S_NOTEQUAL 42
#define S_LOW 43
#define S_BIG 44
#define S_OREQUAL 45
#define S_ANDEQUAL 46
#define S_UPEQUAL 47
#define S_LOWEQUAL 48
#define S_BIGEQUAL 49
#define S_LEFTQUARE 50
#define S_RIGHTQUARE 51
#define S_LEFTBIG 52
#define S_RIGHTBIG 53
#define S_LEFTCIRCLE 54
#define S_RIGHTCIRCLE 55
#define S_DOT 56
#define S_COLON 57
#define S_COMMA 58
#define S_SEMICOLON 59
#define S_BAND 60
#define S_BOR 61
#define S_BNOT 62
#define S_UP 63

#define SYMBOL_LENGTH 64

#define S_GET 64

#define S_NL 65
#define S_INDENT 66
#define S_DEDENT 67
#define S_SYMBOL 68
#define S_NUMBER 69
#define S_NAME 70
#define S_STRING 71
#define S_STATEMENTS 72
#define S_TUPLE 73
#define S_METHODS 74
#define S_EOF 75
#define S_EMPTYDEF 76
#define S_NOTIN 77
#define S_ISNOT 78
#define S_DICT 79
#define S_COMP 80
#define S_SLICE 81
#define S_CALL 82
#define S_REG 83
#define S_MODULE 84
#define S_ARGS 85
#define S_NARGS 86
#define S_DOLLAR 87
#define S_EMPTYQUAD 88  
#define S_EMPTYBIG 89
#define S_LIST 90
#define S_END 91
#define S_BEGIN 92
#define S_LOOP 93

#define MAX_SYMS 100

extern int in_symbol(const char *s, int n);

extern const char* CONSTR[];

