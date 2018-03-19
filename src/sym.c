#include "sym.h"
#include <string.h>

const char* CONSTR[] = {
	"",  // 0
	"def",  // 1
	"class",  // 2
	"yield",  // 3
	"return",  // 4
	"pass",  // 5
	"and",  // 6
	"or",  // 7
	"not",  // 8
	"in",  // 9
	"import",  // 10
	"is",  // 11
	"while",  // 12
	"break",  // 13
	"for",  // 14
	"continue",  // 15
	"if",  // 16
	"else",  // 17
	"elif",  // 18
	"try",  // 19
	"except",  // 20
	"raise",  // 21
	"True",  // 22
	"False",  // 23
	"None",  // 24
	"global",  // 25
	"del",  // 26
	"from",  // 27
	"-",  // 28
	"+",  // 29
	"*",  // 30
	"**",  // 31
	"/",  // 32
	"%",  // 33
	"<<",  // 34
	">>",  // 35
	"-=",  // 36
	"+=",  // 37
	"*=",  // 38
	"/=",  // 39
	"=",  // 40
	"==",  // 41
	"!=",  // 42
	"<",  // 43
	">",  // 44
	"|=",  // 45
	"&=",  // 46
	"^=",  // 47
	"<=",  // 48
	">=",  // 49
	"[",  // 50
	"]",  // 51
	"{",  // 52
	"}",  // 53
	"(",  // 54
	")",  // 55
	".",  // 56
	":",  // 57
	",",  // 58
	";",  // 59
	"&",  // 60
	"|",  // 61
	"!",  // 62
	"^",  // 63
	"get",  // 64
	"nl",  // 65
	"indent",  // 66
	"dedent",  // 67
	"symbol",  // 68
	"number",  // 69
	"name",  // 70
	"string",  // 71
	"statements",  // 72
	"tuple",  // 73
	"methods",  // 74
	"eof",  // 75
	"():",  // 76
	"notin",  // 77
	"isnot",  // 78
	"dict",  // 79
	"comp",  // 80
	"slice",  // 81
	"call",  // 82
	"reg",  // 83
	"module",  // 84
	"args",  // 85
	"nargs", // 86
	"$",  // 87
	"[]",  // 88
	"{}",  // 89
	"list",  // 90
	"end",  // 91
	"begin",  // 92
	"loop",  // 93
};

int in_symbol(const char *s, int n)
{
	for (int i = 1; i < SYMBOL_LENGTH; i++)
	{
		const char* b = CONSTR[i];
		int bl = strlen(b);
		if (bl == n)
		{
			int j = 0;
			for (; j < n;j++)
			{
				if (b[j] != s[j])
					break;
			}
			if (j == n) return i;
		}
	}
	return 0;
}