#include <stdio.h>
#include "lp.h"
#include <stdlib.h>

/* INCLUDE */

void test_0();
void test_1();

int main(int argc, char *argv[]) {
	FILE *f;
    long l;
    lp_obj *c, *g, *r;  //
    char *s, *fname, *rc;
	int size, result;
	struct stat stbuf;

	if (argc < 2)
		return 0;

    test_0();
    test_1();

    lp_vm *lp = lp_init(argc,argv);
    /* INIT */

    fname = argv[1];
    stat(fname, &stbuf);
    l = stbuf.st_size;
    f = fopen(fname, "rb");
    if (!f) {
        return 0;
    }
    s = (char*)malloc(l+1);
    fread(s,1,l,f);
/*    if (rr !=l) { printf("hmmn: %d %d\n",rr,(int)l); }*/
    fclose(f);
	s[l] = '\0';

    g = lp_dict(lp);
	lp_setkv(lp, g, lp_string(lp, "__file__"), lp_string(lp, fname));
    lp_setkv(lp, g, lp_string(lp, "__name__"), lp_string(lp, "__main__"));
    //lp_set(lp,d,lp_string("key"),lp_number(e.key.keysym.sym));
	rc = compile(fname, s, &size, &result);
	if (!result)
	{
		printf("%s\n", rc);
		free(rc);
		getchar();
		return 1;
	}
	c = lp_string_copy(lp, rc, size);
	free(rc);
    r = lp_exec(lp, c, g);
	if (!r) lp_print_stack(lp);
	else LP_OBJ_DEC(r);
	//c = lp_disasm(lp, c);
	//printf(c->string.val);
	//LP_OBJ_DEC(c);

    lp_deinit(lp);
	printf("%s\n", "lp quit...");
	getchar();
    return(0);
}

/**/
