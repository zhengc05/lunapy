#include "sym.h"
#include "tokenize.h"
#include <assert.h>
#include <setjmp.h>

int test_show(char *buffer, int st, struct Token* t, bool is_parse)
{
	const char* s;
	int i = 0;
	switch (t->type)
	{
	case S_STRING:
		buffer[st++] = '"';
		while (t->vn[i])
		{
			buffer[st+i] = t->vn[i];
			i++;
		}
		buffer[st+i] = '"';
		return st+i+1;
	case S_NUMBER:
		while (t->vn[i])
		{
			buffer[st+i] = t->vn[i];
			i++;
		}
		return st+i;
	case S_SYMBOL:
		s = CONSTR[t->vs];
		while (s[i])
		{
			buffer[st+i] = s[i];
			i++;
		}
		return st+i;
	case S_NAME:
		if (!is_parse)
			buffer[st++] = '$';
		while (t->vn[i])
		{
			buffer[st+i] = t->vn[i];
			i++;
		}
		return st+i;
	}

	s = CONSTR[t->type];
	while (s[i])
	{
		buffer[st+i] = s[i];
		i++;
	}
	return st+i;
}

void test_tokenize(const char* s, const char* exp)
{
	char buffer[1024];
	struct CompileState c;
	struct TListItem* ts;
	memset(&c, 0, sizeof(struct CompileState));
	ts = tokenize(&c, s).head;
	int i = 0;
	while (ts)
	{
		i = test_show(buffer, i, ts->t, false);
		ts = ts->next;
		if (ts)
			buffer[i++] = ' ';
	}
	buffer[i] = '\0';

	if (strcmp(buffer, exp))
	{
		assert(0);
	}
}

void test_0()
{
	test_tokenize("234","234");
    test_tokenize("234.234","234.234");
    test_tokenize("phil","$phil");
    test_tokenize("_phil234","$_phil234");
    test_tokenize("'phil'","\"phil\"");
    test_tokenize("\"phil\"","\"phil\"");
    test_tokenize("'phil' x","\"phil\" $x");
    test_tokenize("#comment","");
    test_tokenize("and","and");
    test_tokenize("=","=");
    test_tokenize("()","( )");
    test_tokenize("(==)","( == )");
    test_tokenize("phil=234","$phil = 234");
    test_tokenize("a b","$a $b");
    test_tokenize("a\nb","$a nl $b");
    test_tokenize("a\n    b","$a nl indent $b dedent");
    test_tokenize("a\n    b\n        c", "$a nl indent $b nl indent $c dedent dedent");
    test_tokenize("a\n    b\n    c", "$a nl indent $b nl $c dedent");
    test_tokenize("a\n    b\n        \n      c", "$a nl indent $b nl nl indent $c dedent dedent");
    test_tokenize("a\n    b\nc", "$a nl indent $b nl dedent $c");
    test_tokenize("a\n  b\n    c\nd", "$a nl indent $b nl indent $c nl dedent dedent $d");
    test_tokenize("(\n  )","( )");
    test_tokenize("  x","indent $x dedent");
    test_tokenize("  #","");
    test_tokenize("None","None");
}

int test_lisp(char* buffer, int st, struct Token* t)
{
	int i = st;
	struct TListItem* item;
	item = t->items.head;
	if (!item)
	{
		i = test_show(buffer, i, t, true);
		return i;
	}
	buffer[i++] = '(';
	i = test_show(buffer, i, t, true);
	while (item)
	{
		buffer[i++] = ' ';
		i = test_lisp(buffer, i, item->t);
		item = item->next;
	}
	buffer[i++] = ')';
	return i;
}

void test_parse(const char* s, const char* ex)
{
	char buffer[1024];
	struct CompileState c;
	struct TList ts;
	struct Token* t;
	memset(&c, 0, sizeof(struct CompileState));
	ts = tokenize(&c, s);
	t = parse(&c, s, ts);
	int i = 0;

	memset(buffer, 0, 1024);
	i = test_lisp(buffer, i, t);
	buffer[i++] = '\0';

	if (strcmp(buffer, ex))
	{
		assert(0);
	}
}

void set_error_jmp(jmp_buf b);
void test_unparse(const char* s)
{
	struct TList tokens;
	struct Token* t;
	struct CompileState c;
	memset(&c, 0, sizeof(struct CompileState));
	if (setjmp(c.error_jmp_buf))
	{
		return;
	}

	tokens = tokenize(&c, s);
	t = parse(&c, s, tokens);
	assert(0);
}

void test_1()
{
	test_parse("2+4*3", "(+ 2 (* 4 3))");
    test_parse("4*(2+3)", "(* 4 (+ 2 3))");
    test_parse("(2+3)*4", "(* (+ 2 3) 4)");
    test_parse("1<2", "(< 1 2)");
    test_parse("x=3", "(= x 3)");
    test_parse("x = 2*3", "(= x (* 2 3))");
    test_parse("x = y", "(= x y)");
    test_parse("2,3", "(tuple 2 3)");
    test_parse("2,3,4", "(tuple 2 3 4)");
    test_parse("[]", "list");
    test_parse("[1]", "(list 1)");
    test_parse("[2,3,4]", "(list 2 3 4)");
    test_parse("print(3)", "(call print 3)");
    test_parse("print()", "(call print)");
    test_parse("print(2,3)", "(call print 2 3)");
    test_parse("def fnc():pass", "(def fnc (): pass)");
    test_parse("def fnc(x):pass", "(def fnc ((): x) pass)");
    test_parse("def fnc(x,y):pass", "(def fnc ((): x y) pass)");
    test_parse("x\ny\nz", "(statements x y z)");
    test_parse("return x", "(return x)");
    test_parse("print(test(2,3))", "(call print (call test 2 3))");
    test_parse("x.y", "(get x \"y\")");
    test_parse("get(2).x", "(get (call get 2) \"x\")");
    test_parse("{}", "dict");
    test_parse("True", "True");
    test_parse("False", "False");
    test_parse("None", "None");
    test_parse("while 1:pass", "(while 1 pass)");
    test_parse("for x in y:pass", "(for x y pass)");
    test_parse("print(\"x\")", "(call print \"x\")");
    test_parse("if 1: pass", "(if (elif 1 pass))");
    test_parse("x = []", "(= x list)");
    test_parse("x[1]", "(get x 1)");
    test_parse("import sdl", "(import sdl)");
    test_parse("2 is 3", "(is 2 3)");
    test_parse("2 is not 3", "(isnot 2 3)");
    test_parse("x is None", "(is x None)");
    test_parse("2 is 3 or 4 is 5", "(or (is 2 3) (is 4 5))");
    test_parse("e.x == 3 or e.x == 4", "(or (== (get e \"x\") 3) (== (get e \"x\") 4))");
    test_parse("if 1==2: 3\nelif 4:5\nelse:6", "(if (elif (== 1 2) 3) (elif 4 5) (else 6))");
    test_parse("x = y(2)*3 + y(4)*5", "(= x (+ (* (call y 2) 3) (* (call y 4) 5)))");
    test_parse("x(1,2)+y(3,4)", "(+ (call x 1 2) (call y 3 4))");
    test_parse("x(a,b,c[d])", "(call x a b (get c d))");
    test_parse("x(1,2)*j+y(3,4)*k+z(5,6)*l", "(+ (+ (* (call x 1 2) j) (* (call y 3 4) k)) (* (call z 5 6) l))");
    test_parse("a = b.x/c * 2 - 1", "(= a (- (* (/ (get b \"x\") c) 2) 1))");
    test_parse("for x in y: z", "(for x y z)");

    test_parse("min(255,n*2)","(call min 255 (* n 2))");
    test_parse("c = pal[i*8]","(= c (get pal (* i 8)))");
    test_parse("{x:y,a:b}","(dict x y a b)");
    test_parse("x[1:2:3]","(get x (slice 1 2 3))");
    test_parse("x - -234","(- x -234)");
    test_parse("x - -y","(- x (- 0 y))");
    test_parse("x = ((y*4)-2)","(= x (- (* y 4) 2))");
    
    test_parse("print([1,2,\"OK\",4][-3:3][1])","(call print (get (get (list 1 2 \"OK\" 4) (slice -3 3)) 1))");
    
    test_parse("a,b = 1,2","(= (tuple a b) (tuple 1 2))");
    test_parse("class C: pass","(class C (methods pass))");
    test_parse("def test(*v): pass","(def test ((): (args v)) pass)");
    test_parse("def test(**v): pass","(def test ((): (nargs v)) pass)");
    test_parse("test(*v)","(call test (args v))");
    test_parse("test(**v)","(call test (nargs v))");
    test_parse("def test(x=y): pass","(def test ((): (= x y)) pass)");
    test_parse("test(x=y)","(call test (= x y))");
    test_parse("def test(y=\"K\",x=\"Z\"): pass","(def test ((): (= y \"K\") (= x \"Z\")) pass)");
    test_parse("return x+y","(return (+ x y))");
    test_parse("if \"a\" is not \"b\": pass","(if (elif (isnot \"a\" \"b\") pass))");
    test_parse("z = 0\nfor x in y: pass","(statements (= z 0) (for x y pass))");
    test_parse("for k in {\"OK\":0}: pass","(for k (dict \"OK\" 0) pass)");
    test_parse("print(test(10,3,z=50000,*[200],**{\"x\":4000}))","(call print (call test 10 3 (= z 50000) (args (list 200)) (nargs (dict \"x\" 4000))))");
    test_parse("x=\"OK\";print(x)","(statements (= x \"OK\") (call print x))");
    test_parse("[(1,3)]","(list (tuple 1 3))");
    test_parse("x[:]","(get x (slice None None))");
    test_parse("x[:1]","(get x (slice None 1))");
    test_parse("x[1:]","(get x (slice 1 None))");
    test_parse("return\nx","(statements return x)");
    test_parse("\"test\"","\"test\"");
    test_parse("return a,b","(return (tuple a b))");
    
    // this should throw an error - bug #26
    test_unparse("while 1:\npass");
        
    // test cases for python 2.x print statements - bug #17
    // since this is python 2.x syntax it should oulput an syntax Exception
    
    // mini-block style
    test_unparse("def x(): print \"OK\"");

    // block level
    test_unparse("def x():\n    print \"OK\"");

    // module level
    test_unparse("print \"OK\"");

}

