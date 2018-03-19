#include <sys/timeb.h>

#include <errno.h>
#include "lp.h"
#include "lp_internal.h"

#ifdef WIN32
#define HAVE_FTIME
#endif

/* Implement floattime() for various platforms */

static double floattime(void)
{
	/* There are three ways to get the time:
	(1) gettimeofday() -- resolution in microseconds
	(2) ftime() -- resolution in milliseconds
	(3) time() -- resolution in seconds
	In all cases the return value is a float in seconds.
	Since on some systems (e.g. SCO ODT 3.0) gettimeofday() may
	fail, so we fall back on ftime() or time().
	Note: clock resolution does not imply clock accuracy! */
#ifdef HAVE_GETTIMEOFDAY
{
	struct timeval t;
#ifdef GETTIMEOFDAY_NO_TZ
	if (gettimeofday(&t) == 0)
		return (double)t.tv_sec + t.tv_usec*0.000001;
#else /* !GETTIMEOFDAY_NO_TZ */
	if (gettimeofday(&t, (struct timezone *)NULL) == 0)
		return (double)t.tv_sec + t.tv_usec*0.000001;
#endif /* !GETTIMEOFDAY_NO_TZ */
}

#endif /* !HAVE_GETTIMEOFDAY */
{
#if defined(HAVE_FTIME)
	struct timeb t;
	ftime(&t);
	return (double)t.time + (double)t.millitm * (double)0.001;
#else /* !HAVE_FTIME */
	time_t secs;
	time(&secs);
	return (double)secs;
#endif /* !HAVE_FTIME */
}
}

/*
 * modf(x)
 *
 * return a pair (r, y). r is the integral part of
 * x and y is the fractional part of x, both holds
 * the same sign as x.
 */
static lp_obj* time_time(LP) {
	return lp_number_from_double(lp, floattime());
}



/*
 * init math module, namely, set its dictionary
 */
void time_init(LP)
{
    /*
     * new a module dict for math
     */
    lp_obj* time_mod = lp_dict(lp);

    /*
     * bind math functions to math module
     */
    lp_setkv(lp, time_mod, lp_string(lp, "time"), lp_fnc(lp, time_time));

    /*
     * bind special attributes to math module
     */
    lp_setkv(lp, time_mod, lp_string(lp, "__doc__"),
            lp_string(lp, 
                "This module is always available.  It provides access to the\n"
                "time-rel functions defined by the C standard."));
    lp_setkv(lp, time_mod, lp_string(lp, "__name__"), lp_string(lp, "time"));
    lp_setkv(lp, time_mod, lp_string(lp, "__file__"), lp_string(lp, __FILE__));

    /*
     * bind to tiny modules[]
     */
    lp_setkv(lp, lp->modules, lp_string(lp, "time"), time_mod);
}