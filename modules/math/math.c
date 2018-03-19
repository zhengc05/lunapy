#include <math.h>
#ifndef M_E
 #define M_E     2.7182818284590452354
#endif
#ifndef M_PI
 #define M_PI    3.14159265358979323846
#endif

#include <errno.h>

#include "lp.h"
#include "lp_internal.h"

/*
 * template for tinypy math functions
 * with one parameter. 
 *
 * @cfunc is the coresponding function name in C
 * math library.
 */
#define LP_MATH_FUNC1(cfunc)                        \
    static lp_obj* math_##cfunc(LP) {                \
        double x = LP_NUM(0);                        \
        double r = 0.0;                             \
                                                    \
        errno = 0;                                  \
        r = cfunc(x);                               \
        if (errno == EDOM || errno == ERANGE) {     \
            lp_raise(0, lp_printf(lp, "%s(x): x=%f "		\
                                        "out of range", #cfunc, x));	\
        }                                           \
                                                    \
        return (lp_number_from_double(lp, r));                      \
    }

/*
 * template for tinypy math functions
 * with two parameters. 
 *
 * @cfunc is the coresponding function name in C
 * math library.
 */
#define LP_MATH_FUNC2(cfunc)                        \
    static lp_obj* math_##cfunc(LP) {                \
        double x = LP_NUM(0);                        \
        double y = LP_NUM(1);                        \
        double r = 0.0;                             \
                                                    \
        errno = 0;                                  \
        r = cfunc(x, y);                            \
        if (errno == EDOM || errno == ERANGE) {     \
            lp_raise(0, lp_printf(lp, "%s(x, y): x=%f,y=%f "	\
                                        "out of range", #cfunc, x, y)); \
        }                                           \
                                                    \
        return (lp_number_from_double(lp, r));                      \
    }


/*
 * PI definition: 3.1415926535897931
 */
static lp_obj*   math_pi;

/*
 * E definition: 2.7182818284590451
 */
static lp_obj*   math_e;

/*
 * acos(x)
 *
 * return arc cosine of x, return value is measured in radians.
 * if x falls out -1 to 1, raise out-of-range exception.
 */
LP_MATH_FUNC1(acos)

/*
 * asin(x)
 *
 * return arc sine of x, measured in radians, actually [-PI/2, PI/2]
 * if x falls out of -1 to 1, raise out-of-range exception
 */
LP_MATH_FUNC1(asin)

/*
 * atan(x)
 *
 * return arc tangent of x, measured in radians,
 */
LP_MATH_FUNC1(atan)

/*
 * atan2(x, y)
 *
 * return arc tangent of x/y, measured in radians.
 * unlike atan(x/y), both the signs of x and y 
 * are considered to determine the quaderant of 
 * the result.
 */
LP_MATH_FUNC2(atan2)

/*
 * ceil(x)
 *
 * return the ceiling of x, i.e, the smallest
 * integer >= x.
 */
LP_MATH_FUNC1(ceil)

/*
 * cos(x)
 *
 * return cosine of x. x is measured in radians.
 */
LP_MATH_FUNC1(cos)

/*
 * cosh(x)
 *
 * return hyperbolic cosine of x.
 */
LP_MATH_FUNC1(cosh)

/*
 * degrees(x)
 *
 * converts angle x from radians to degrees.
 * NOTE: this function is introduced by python,
 * so we cannot wrap it directly in LP_MATH_FUNC1(),
 * here the solution is defining a new 
 * C function - degrees().
 */
static const double degToRad = 
                3.141592653589793238462643383 / 180.0;
static double degrees(double x)
{
    return (x / degToRad);
}

LP_MATH_FUNC1(degrees)

/*
 * exp(x)
 *
 * return the value e raised to power of x.
 * e is the base of natural logarithms.
 */
LP_MATH_FUNC1(exp)

/*
 * fabs(x)
 *
 * return the absolute value of x.
 */
LP_MATH_FUNC1(fabs)

/*
 * floor(x)
 *
 * return the floor of x, i.e, the largest integer <= x
 */
LP_MATH_FUNC1(floor)

/*
 * fmod(x, y)
 *
 * return the remainder of dividing x by y. that is,
 * return x - n * y, where n is the quotient of x/y.
 * NOTE: this function relies on the underlying platform.
 */
LP_MATH_FUNC2(fmod)

/*
 * frexp(x)
 *
 * return a pair (r, y), which satisfies:
 * x = r * (2 ** y), and r is normalized fraction
 * which is laid between 1/2 <= abs(r) < 1.
 * if x = 0, the (r, y) = (0, 0).
 */
static lp_obj* math_frexp(LP) {
    double x = LP_NUM(0);
    int    y = 0;   
    double r = 0.0;
    lp_obj* rList = lp_list(lp);

    errno = 0;
    r = frexp(x, &y);
    if (errno == EDOM || errno == ERANGE) {
        lp_raise(0, lp_printf(lp, "%s(x): x=%f, "
                                    "out of range", frexp, x));
    }

    _lp_list_appendx(lp, rList->list, lp_number_from_double(lp, r));
    _lp_list_appendx(lp, rList->list, lp_number_from_int(lp, y));
    return (rList);
}


/*
 * hypot(x, y)
 *
 * return Euclidean distance, namely,
 * sqrt(x*x + y*y)
 */
LP_MATH_FUNC2(hypot)


/*
 * ldexp(x, y)
 *
 * return the result of multiplying x by 2
 * raised to y.
 */
LP_MATH_FUNC2(ldexp)

/*
 * log(x, [base])
 *
 * return logarithm of x to given base. If base is
 * not given, return the natural logarithm of x.
 * Note: common logarithm(log10) is used to compute 
 * the denominator and numerator. based on fomula:
 * log(x, base) = log10(x) / log10(base).
 */
static lp_obj* math_log(LP) {
    double x = LP_NUM(0);
    lp_obj* b = LP_DEFAULT(1, lp->lp_None);
    double y = 0.0;
    double den = 0.0;   /* denominator */
    double num = 0.0;   /* numinator */
    double r = 0.0;     /* result */

    if (b->type == LP_NONE)
        y = M_E;
    else if (lp_is_number(b))
        y = lp_type_number(lp, b);
    else
        lp_raise(0, lp_printf(lp, "%s(x, [base]): base invalid", "log"));

    errno = 0;
    num = log10(x);
    if (errno == EDOM || errno == ERANGE)
        goto excep;

    errno = 0;
    den = log10(y);
    if (errno == EDOM || errno == ERANGE)
        goto excep;

    r = num / den;

    return (lp_number_from_double(lp, r));

excep:
    lp_raise(0, lp_printf(lp, "%s(x, y): x=%f,y=%f "
                                "out of range", "log", x, y));
}

/*
 * log10(x)
 *
 * return 10-based logarithm of x.
 */
LP_MATH_FUNC1(log10)

/*
 * modf(x)
 *
 * return a pair (r, y). r is the integral part of
 * x and y is the fractional part of x, both holds
 * the same sign as x.
 */
static lp_obj* math_modf(LP) {
    double x = LP_NUM(0);
    double y = 0.0; 
    double r = 0.0;
    lp_obj* rList = lp_list(lp);

    errno = 0;
    r = modf(x, &y);
    if (errno == EDOM || errno == ERANGE) {
        lp_raise(0, lp_printf(lp, "%s(x): x=%f, "
                                    "out of range", "modf", x));
    }

    _lp_list_appendx(lp, rList->list, lp_number_from_double(lp, r));
    _lp_list_appendx(lp, rList->list, lp_number_from_int(lp, y));
    return (rList);
}

/*
 * pow(x, y)
 *
 * return value of x raised to y. equivalence of x ** y.
 * NOTE: conventionally, lp_pow() is the implementation
 * of builtin function pow(); whilst, math_pow() is an
 * alternative in math module.
 */
static lp_obj* math_pow(LP) {
    double x = LP_NUM(0);
    double y = LP_NUM(1);
    double r = 0.0;

    errno = 0;
    r = pow(x, y);
    if (errno == EDOM || errno == ERANGE) {
        lp_raise(0, lp_printf(lp, "%s(x, y): x=%f,y=%f "
                                    "out of range", "pow", x, y));
    }

    return (lp_number_from_double(lp, r));
}


/*
 * radians(x)
 *
 * converts angle x from degrees to radians.
 * NOTE: this function is introduced by python,
 * adopt same solution as degrees(x).
 */
static double radians(double x)
{
    return (x * degToRad);
}

LP_MATH_FUNC1(radians)

/*
 * sin(x)
 *
 * return sine of x, x is measured in radians.
 */
LP_MATH_FUNC1(sin)

/*
 * sinh(x)
 *
 * return hyperbolic sine of x.
 * mathematically, sinh(x) = (exp(x) - exp(-x)) / 2.
 */
LP_MATH_FUNC1(sinh)

/*
 * sqrt(x)
 *
 * return square root of x.
 * if x is negtive, raise out-of-range exception.
 */
LP_MATH_FUNC1(sqrt)

/*
 * tan(x)
 *
 * return tangent of x, x is measured in radians.
 */
LP_MATH_FUNC1(tan)

/*
 * tanh(x)
 * 
 * return hyperbolic tangent of x.
 * mathematically, tanh(x) = sinh(x) / cosh(x).
 */
LP_MATH_FUNC1(tanh)

/*
 * init math module, namely, set its dictionary
 */
void math_init(LP)
{
    /*
     * new a module dict for math
     */
    lp_obj* math_mod = lp_dict(lp);

    /*
     * initialize pi and e
     */
    math_pi = lp_number_from_double(lp, M_PI);
    math_e  = lp_number_from_double(lp, M_E);

    /*
     * bind math functions to math module
     */
    lp_setk(lp, math_mod, lp_string(lp, "pi"), math_pi);
    lp_setk(lp, math_mod, lp_string(lp, "e"), math_e);
    lp_setkv(lp, math_mod, lp_string(lp, "acos"), lp_fnc(lp, math_acos));
    lp_setkv(lp, math_mod, lp_string(lp, "asin"), lp_fnc(lp, math_asin));
    lp_setkv(lp, math_mod, lp_string(lp, "atan"), lp_fnc(lp, math_atan));
    lp_setkv(lp, math_mod, lp_string(lp, "atan2"), lp_fnc(lp, math_atan2));
    lp_setkv(lp, math_mod, lp_string(lp, "ceil"), lp_fnc(lp, math_ceil));
    lp_setkv(lp, math_mod, lp_string(lp, "cos"), lp_fnc(lp, math_cos));
    lp_setkv(lp, math_mod, lp_string(lp, "cosh"), lp_fnc(lp, math_cosh));
    lp_setkv(lp, math_mod, lp_string(lp, "degrees"), lp_fnc(lp, math_degrees));
    lp_setkv(lp, math_mod, lp_string(lp, "exp"), lp_fnc(lp, math_exp));
    lp_setkv(lp, math_mod, lp_string(lp, "fabs"), lp_fnc(lp, math_fabs));
    lp_setkv(lp, math_mod, lp_string(lp, "floor"), lp_fnc(lp, math_floor));
    lp_setkv(lp, math_mod, lp_string(lp, "fmod"), lp_fnc(lp, math_fmod));
    lp_setkv(lp, math_mod, lp_string(lp, "frexp"), lp_fnc(lp, math_frexp));
    lp_setkv(lp, math_mod, lp_string(lp, "hypot"), lp_fnc(lp, math_hypot));
    lp_setkv(lp, math_mod, lp_string(lp, "ldexp"), lp_fnc(lp, math_ldexp));
    lp_setkv(lp, math_mod, lp_string(lp, "log"), lp_fnc(lp, math_log));
    lp_setkv(lp, math_mod, lp_string(lp, "log10"), lp_fnc(lp, math_log10));
    lp_setkv(lp, math_mod, lp_string(lp, "modf"), lp_fnc(lp, math_modf));
    lp_setkv(lp, math_mod, lp_string(lp, "pow"), lp_fnc(lp, math_pow));
    lp_setkv(lp, math_mod, lp_string(lp, "radians"), lp_fnc(lp, math_radians));
    lp_setkv(lp, math_mod, lp_string(lp, "sin"), lp_fnc(lp, math_sin));
    lp_setkv(lp, math_mod, lp_string(lp, "sinh"), lp_fnc(lp, math_sinh));
    lp_setkv(lp, math_mod, lp_string(lp, "sqrt"), lp_fnc(lp, math_sqrt));
    lp_setkv(lp, math_mod, lp_string(lp, "tan"), lp_fnc(lp, math_tan));
    lp_setkv(lp, math_mod, lp_string(lp, "tanh"), lp_fnc(lp, math_tanh));

    /*
     * bind special attributes to math module
     */
    lp_setkv(lp, math_mod, lp_string(lp, "__doc__"), 
            lp_string(lp, 
                "This module is always available.  It provides access to the\n"
                "mathematical functions defined by the C standard."));
    lp_setkv(lp, math_mod, lp_string(lp, "__name__"), lp_string(lp, "math"));
    lp_setkv(lp, math_mod, lp_string(lp, "__file__"), lp_string(lp, __FILE__));

    /*
     * bind to tiny modules[]
     */
    lp_setkv(lp, lp->modules, lp_string(lp, "math"), math_mod);
}