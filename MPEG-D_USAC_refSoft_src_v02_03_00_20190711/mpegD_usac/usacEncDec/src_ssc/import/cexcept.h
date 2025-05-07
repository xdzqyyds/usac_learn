/*===
cexcept.h amc.0.5.3 (2000-Apr-01-Sat)
Adam M. Costello <amc@cs.berkeley.edu>

An interface for exception-handling in ANSI C, based on the ideas of
Cosmin Truta <cosmin@cs.toronto.edu>.

    Copyright (c) 2000 Adam M. Costello and Cosmin Truta.  Everyone is
    hereby granted permission to do whatever they like with this file,
    provided that if they modify it they take reasonable steps to avoid
    confusing or misleading people about the authors and version of the
    derived file.  The copyright holders make no guarantees about the
    correctness of this file, and are not responsible for any damage
    resulting from its use.

You don't normally want .c files to include this header file directly.
Instead, create a wrapper header file that includes this header file and
then invokes the define_exception_type macro (see below), and let your
.c files include that header file.

The interface consists of one type, one well-known name, and five
macros.


define_exception_type(type_name);

    This macro is used like an external declaration.  It specifies
    the type of object that gets copied from the exception thrower to
    the exception catcher.  The type_name can be any type that can be
    copied, that is, anything except a function, array, incomplete
    struct/union, or void.


struct exception_context;

    This type may be used after the define_exception_type() macro has
    been invoked.  A struct exception_context must be known to both the
    thrower and the catcher.  It is expected that there be one struct
    exception_context for each thread that uses exceptions.  It would
    certainly be dangerous for multiple threads to access the same
    context, and would probably not be useful for one thread to use
    multiple contexts.  The application can allocate this structure in
    any way it pleases--automatic, static, or dynamic.  The application
    programmer should pretend not to know the structure members, which
    are subject to change.


struct exception_context *the_exception_context;

    It is the application's responsibility to make sure that the
    name the_exception_context refers to the address of a mutable
    (non-constant) struct exception_context before invoking any of the
    macros described below.  Subject to that constraint, the application
    may declare a variable of that name anywhere it likes (inside a
    function, in a parameter list, or externally), and may use whatever
    type qualifiers it likes (static, extern, const, etc).  Examples:

    static struct exception_context * const the_exception_context = &foo;
    { struct exception_context *the_exception_context = bar; ... }
    int blah(struct exception_context *the_exception_context, ...) ...
    extern struct exception_context the_exception_context[1];

    The last example illustrates a trick that avoids creating a pointer
    object separate from the structure object.

    The name could even be a macro, for example:

    struct exception_context ec_array[numthreads];
    #define the_exception_context (ec_array + thread_id)

    Be aware that the the_exception_context is referred to a number of
    times by the Try/Catch/Throw macros, so it shouldn't be expensive or
    have side effects.  The expansion must be a drop-in replacement for
    an identifier, so it's safest to put parentheses around it.


void init_exception_context(void);

    This macro is used like a function.  It must be called once after
    the struct exception_context is allocated, before the first
    Try/Catch statement is encountered.


Try statement
Catch (expression) statement

    The Try/Catch/Throw macros are capitalized in order to avoid
    confusion with the C++ keywords, which have subtly different
    semantics.

    A Try/Catch statement has a syntax similar to an if/else statement,
    except that the parenthesized expression goes after the second
    keyword rather than the first.  As with if/else, there are two
    clauses, each of which may be a simple statement ending with a
    semicolon or a brace-enclosed compound statement.  But whereas
    the else clause is optional, the Catch clause is required.  The
    expression must be an lvalue (something capable of being assigned
    to) compatible with the type passed to define_exception_type().

    If a Throw that uses the same exception context as the Try/Catch is
    executed within the Try clause (typically within a function called
    by the Try clause), and the exception is not caught by a nested
    Try/Catch statement, then a copy of the exception will be assigned
    to the expression, and control will jump to the Catch clause.  If
    no such Throw is executed, then the Catch clause is not executed,
    the assignment is not performed, and the expression is not even
    evaluated (which is significant if it has side effects).

    IMPORTANT: return, goto, continue, and break statements must not be
    used to jump out of a Try clause--the Catch keyword must be reached
    (but jumping out of the Catch clause is okay).  Also, the values of
    any non-volatile automatic variables changed within the Try clause
    are undefined after an exception is caught.


Throw expression;

    A Throw statement is very much like a return statement, except that
    the expression is required.  Whereas return jumps back to the place
    where the current function was called, Throw jumps back to the Catch
    clause of the innermost enclosing Try clause.  The expression must
    be compatible with the type passed to define_exception_type().  The
    exception must be caught, otherwise the program may crash.

    Slight limitation:  The expression cannot be a comma-expression (but
    no one would want to use a comma-expression here anyway).


Everything below this point is for the benefit of the compiler.  The
application programmer should pretend not to know any of it, because it
is subject to change.

===*/


#ifndef CEXCEPT_H
#define CEXCEPT_H


#include <setjmp.h>

#define define_exception_type(etype) \
struct exception_context { \
  struct exception__jmp_buf *last; \
  int caught; \
  etype exception; \
}

struct exception__jmp_buf {
  jmp_buf env;
};

/* The purpose of this wrapper struct around jmp_buf is to guard     */
/* against compilers that fail to consider *&x to be equivalent to x */
/* (not x[1]) when x is an array, because jmp_buf is an array type.  */
/* In ANSI C this is no problem, but it's a subtle point, so we'll   */
/* just be paranoid and use the wrapper struct.                      */

#define init_exception_context() ((void)(the_exception_context->last = 0))

#define Try \
  { \
    struct exception__jmp_buf *exception__p, exception__j; \
    exception__p = the_exception_context->last; \
    the_exception_context->last = &exception__j; \
    if (setjmp(exception__j.env) == 0) { \
      if (&exception__j)

/* We use &exception__j rather than 1 to appease compilers that       */
/* warn about constant expressions inside the if().  Most compilers   */
/* should still recognize that &exception__j is always true and avoid */
/* generating test code.                                              */

#define Catch(e) \
      else { } \
      the_exception_context->caught = 0; \
    } \
    else the_exception_context->caught = 1; \
    the_exception_context->last = exception__p; \
  } \
  if (!the_exception_context->caught || \
      ((e) = the_exception_context->exception, 0)) { } \
  else

/* Try ends with if(), and Catch begins and ends with else.  */
/* This ensures that the Try/Catch syntax is really the same */
/* as the if/else syntax.                                    */

#define Throw \
  for (;; longjmp(the_exception_context->last->env, 1)) \
    the_exception_context->exception =


#endif /* CEXCEPT_H */
