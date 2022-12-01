/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include <argp.h>
#include <ieee754.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/printf_specifier.h"
#include "interflop-stdlib/common/float_const.h"
#include "interflop-stdlib/fma/fmaqApprox.h"
#include "interflop-stdlib/interflop.h"
#include "interflop-stdlib/iostream/logger.h"
#include "interflop_ieee.h"

typedef enum {
  KEY_DEBUG = 'd',
  KEY_DEBUG_BINARY = 'b',
  KEY_NO_BACKEND_NAME = 's',
  KEY_PRINT_NEW_LINE = 'n',
  KEY_COUNT_OP = 'o',
  KEY_PRINT_SUBNORMAL_NORMALIZED,
} key_args;

static const char key_debug_str[] = "debug";
static const char key_debug_binary_str[] = "debug-binary";
static const char key_no_backend_name_str[] = "no-backend-name";
static const char key_print_new_line_str[] = "print-new-line";
static const char key_print_subnormal_normalized_str[] =
    "print-subnormal-normalized";
static const char key_count_op[] = "count-op";

typedef enum {
  ARITHMETIC = 0,
  COMPARISON = 1,
  CAST = 2,
  FMA = 3,
} operation_type;

#define STRING_MAX 256

#define FMT(X) _Generic(X, float : "b", double : "lb")
#define FMT_SUBNORMAL_NORMALIZED(X) _Generic(X, float : "#b", double : "#lb")

/* inserts the string <str_to_add> at position i */
/* increments i by the size of str_to_add */
void insert_string(char *dst, char *str_to_add, int *i) {
  int j = 0;
  do {
    dst[*i] = str_to_add[j];
  } while ((*i)++, j++, str_to_add[j] != '\0');
}

/* Auxiliary function to debug print that prints  */
/* a new line if requested by option --print-new-line  */
void debug_print_aux(void *context, char *fmt, va_list argp) {
  vfprintf(stderr, fmt, argp);
  if (((ieee_context_t *)context)->print_new_line) {
    interflop_fprintf(stderr, "\n");
  }
}

/* Debug print which replace %g by specific binary format %b */
/* if option --debug-binary is set */
void debug_print(void *context, char *fmt_flt, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (((ieee_context_t *)context)->debug) {
    debug_print_aux(context, fmt, ap);
  } else {
    char new_fmt[STRING_MAX] = "";
    int i = 0, j = 0;
    do {
      switch (fmt[i]) {
      case 'g':
        insert_string(new_fmt, fmt_flt, &j);
        break;
      default:
        new_fmt[j] = fmt[i];
        j++;
        break;
      }
    } while (fmt[i++] != '\0');
    new_fmt[j] = '\0';
    debug_print_aux(context, new_fmt, ap);
  }
  va_end(ap);
}

#define DEBUG_HEADER "Decimal "
#define DEBUG_BINARY_HEADER "Binary "

/* This macro print the debug information for a, b and c */
/* the debug_print function handles automatically the format */
/* (decimal or binary) depending on the context */
#define DEBUG_PRINT(context, typeop, op, a, b, c, d)                           \
  {                                                                            \
    bool debug = ((ieee_context_t *)context)->debug ? true : false;            \
    bool debug_binary =                                                        \
        ((ieee_context_t *)context)->debug_binary ? true : false;              \
    bool subnormal_normalized =                                                \
        ((ieee_context_t *)context)->print_subnormal_normalized ? true         \
                                                                : false;       \
    if (debug || debug_binary) {                                               \
      bool print_header =                                                      \
          (((ieee_context_t *)context)->no_backend_name) ? false : true;       \
      char *header = (debug) ? DEBUG_HEADER : DEBUG_BINARY_HEADER;             \
      char *a_float_fmt =                                                      \
          (subnormal_normalized) ? FMT_SUBNORMAL_NORMALIZED(a) : FMT(a);       \
      char *b_float_fmt =                                                      \
          (subnormal_normalized) ? FMT_SUBNORMAL_NORMALIZED(b) : FMT(b);       \
      char *c_float_fmt =                                                      \
          (subnormal_normalized) ? FMT_SUBNORMAL_NORMALIZED(c) : FMT(c);       \
      char *d_float_fmt =                                                      \
          (subnormal_normalized) ? FMT_SUBNORMAL_NORMALIZED(d) : FMT(d);       \
      if (print_header) {                                                      \
        if (((ieee_context_t *)context)->print_new_line)                       \
          logger_info("%s\n", header);                                         \
        else                                                                   \
          logger_info("%s", header);                                           \
      }                                                                        \
      if (typeop == ARITHMETIC) {                                              \
        debug_print(context, a_float_fmt, "%g %s ", a, a, op);                 \
        debug_print(context, b_float_fmt, "%g -> ", b);                        \
        debug_print(context, c_float_fmt, "%g\n", c);                          \
      } else if (typeop == COMPARISON) {                                       \
        debug_print(context, a_float_fmt, "%g [%s] ", a, op);                  \
        debug_print(context, b_float_fmt, "%g -> %s\n", b,                     \
                    c ? "true" : "false");                                     \
      } else if (typeop == CAST) {                                             \
        debug_print(context, a_float_fmt, "%g %s -> ", a, op);                 \
        debug_print(context, b_float_fmt, "%g\n", b);                          \
      } else if (typeop == FMA) {                                              \
        debug_print(context, a_float_fmt, "%g * ", a);                         \
        debug_print(context, b_float_fmt, "%g + ", b);                         \
        debug_print(context, c_float_fmt, "%g -> ", c);                        \
        debug_print(context, d_float_fmt, "%g\n", d);                          \
      }                                                                        \
    }                                                                          \
  }

static inline void debug_print_float(void *context, const operation_type typeop,
                                     const char *op, const float a,
                                     const float b, const float c) {

  DEBUG_PRINT(context, typeop, op, a, b, c, (float)0);
}

static inline void debug_print_double(void *context,
                                      const operation_type typeop,
                                      const char *op, const double a,
                                      const double b, const double c) {
  DEBUG_PRINT(context, typeop, op, a, b, c, (double)0);
}

static inline void debug_print_cast_double_to_float(void *context,
                                                    const operation_type typeop,
                                                    const char *op,
                                                    const double a,
                                                    const float b) {
  DEBUG_PRINT(context, typeop, op, a, b, (float)0, (float)0);
}

static inline void debug_print_fma_float(void *context,
                                         const operation_type typeop,
                                         const char *op, const float a,
                                         const float b, const float c,
                                         const float d) {
  DEBUG_PRINT(context, typeop, op, a, b, c, d);
}

static inline void debug_print_fma_double(void *context,
                                          const operation_type typeop,
                                          const char *op, const double a,
                                          const double b, const double c,
                                          const double d) {
  DEBUG_PRINT(context, typeop, op, a, b, c, d);
}

/* Set C to the result of the comparison P between A and B */
/* Set STR to name of the comparison  */
#define SELECT_FLOAT_CMP(A, B, C, P, STR)                                      \
  switch (P) {                                                                 \
  case FCMP_FALSE:                                                             \
    *C = false;                                                                \
    STR = "FCMP_FALSE";                                                        \
    break;                                                                     \
  case FCMP_OEQ:                                                               \
    *C = ((A) == (B));                                                         \
    STR = "FCMP_OEQ";                                                          \
    break;                                                                     \
  case FCMP_OGT:                                                               \
    *C = isgreater(A, B);                                                      \
    STR = "FCMP_OGT";                                                          \
    break;                                                                     \
  case FCMP_OGE:                                                               \
    *C = isgreaterequal(A, B);                                                 \
    STR = "FCMP_OGE";                                                          \
    break;                                                                     \
  case FCMP_OLT:                                                               \
    *C = isless(A, B);                                                         \
    STR = "FCMP_OLT";                                                          \
    break;                                                                     \
  case FCMP_OLE:                                                               \
    *C = islessequal(A, B);                                                    \
    STR = "FCMP_OLE";                                                          \
    break;                                                                     \
  case FCMP_ONE:                                                               \
    *C = islessgreater(A, B);                                                  \
    STR = "FCMP_ONE";                                                          \
    break;                                                                     \
  case FCMP_ORD:                                                               \
    *C = !isunordered(A, B);                                                   \
    STR = "FCMP_ORD";                                                          \
    break;                                                                     \
  case FCMP_UEQ:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UEQ";                                                          \
    break;                                                                     \
  case FCMP_UGT:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UGT";                                                          \
    break;                                                                     \
  case FCMP_UGE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UGE";                                                          \
    break;                                                                     \
  case FCMP_ULT:                                                               \
    *C = isunordered(A, B);                                                    \
    str = "FCMP_ULT";                                                          \
    break;                                                                     \
  case FCMP_ULE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_ULE";                                                          \
    break;                                                                     \
  case FCMP_UNE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UNE";                                                          \
    break;                                                                     \
  case FCMP_UNO:                                                               \
    *C = isunordered(A, B);                                                    \
    str = "FCMP_UNO";                                                          \
    break;                                                                     \
  case FCMP_TRUE:                                                              \
    *c = true;                                                                 \
    str = "FCMP_TRUE";                                                         \
    break;                                                                     \
  }

void INTERFLOP_IEEE_API(add_float)(const float a, const float b, float *c,
                                   void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a + b;
  if (my_context->count_op)
    my_context->add_count++;
  debug_print_float(context, ARITHMETIC, "+", a, b, *c);
}

void INTERFLOP_IEEE_API(sub_float)(const float a, const float b, float *c,
                                   void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a - b;
  if (my_context->count_op)
    my_context->sub_count++;
  debug_print_float(context, ARITHMETIC, "-", a, b, *c);
}

void INTERFLOP_IEEE_API(mul_float)(const float a, const float b, float *c,
                                   void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a * b;
  if (my_context->count_op)
    my_context->mul_count++;
  debug_print_float(context, ARITHMETIC, "*", a, b, *c);
}

void INTERFLOP_IEEE_API(div_float)(const float a, const float b, float *c,
                                   void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a / b;
  if (my_context->count_op)
    my_context->div_count++;
  debug_print_float(context, ARITHMETIC, "/", a, b, *c);
}

void INTERFLOP_IEEE_API(cmp_float)(const enum FCMP_PREDICATE p, const float a,
                                   const float b, int *c, void *context) {
  char *str = "";
  SELECT_FLOAT_CMP(a, b, c, p, str);
  debug_print_float(context, COMPARISON, str, a, b, *c);
}

void INTERFLOP_IEEE_API(add_double)(const double a, const double b, double *c,
                                    void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a + b;
  if (my_context->count_op)
    my_context->add_count++;
  debug_print_double(context, ARITHMETIC, "+", a, b, *c);
}

void INTERFLOP_IEEE_API(sub_double)(const double a, const double b, double *c,
                                    void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a - b;
  if (my_context->count_op)
    my_context->sub_count++;
  debug_print_double(context, ARITHMETIC, "-", a, b, *c);
}

void INTERFLOP_IEEE_API(mul_double)(const double a, const double b, double *c,
                                    void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a * b;
  if (my_context->count_op)
    my_context->mul_count++;
  debug_print_double(context, ARITHMETIC, "*", a, b, *c);
}

void INTERFLOP_IEEE_API(div_double)(const double a, const double b, double *c,
                                    void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;
  *c = a / b;
  if (my_context->count_op)
    my_context->div_count++;
  debug_print_double(context, ARITHMETIC, "/", a, b, *c);
}

void INTERFLOP_IEEE_API(cmp_double)(const enum FCMP_PREDICATE p, const double a,
                                    const double b, int *c, void *context) {
  char *str = "";
  SELECT_FLOAT_CMP(a, b, c, p, str);
  debug_print_double(context, COMPARISON, str, a, b, *c);
}

void INTERFLOP_IEEE_API(cast_double_to_float)(double a, float *b,
                                              void *context) {
  *b = (float)a;
  debug_print_cast_double_to_float(context, CAST, "(float)", a, *b);
}

void INTERFLOP_IEEE_API(fma_float)(float a, float b, float c, float *res,
                                   void *context) {
  *res = (float)fmaApprox(a, b, c);
  debug_print_fma_float(context, FMA, "fma", a, b, c, *res);
}

void INTERFLOP_IEEE_API(fma_double)(double a, double b, double c, double *res,
                                    void *context) {
  *res = fmaApprox(a, b, c);
  debug_print_fma_double(context, FMA, "fma", a, b, c, *res);
}

static struct argp_option options[] = {
    {key_debug_str, KEY_DEBUG, 0, 0, "enable debug output", 0},
    {key_debug_binary_str, KEY_DEBUG_BINARY, 0, 0, "enable binary debug output",
     0},
    {key_no_backend_name_str, KEY_NO_BACKEND_NAME, 0, 0,
     "do not print backend name in debug output", 0},
    {key_print_new_line_str, KEY_PRINT_NEW_LINE, 0, 0,
     "add a new line after debug ouput", 0},
    {key_print_subnormal_normalized_str, KEY_PRINT_SUBNORMAL_NORMALIZED, 0, 0,
     "normalize subnormal numbers", 0},
    {key_count_op, KEY_COUNT_OP, 0, 0, "enable operation count output", 0},
    {0}};

static error_t parse_opt(int key, __attribute__((unused)) char *arg,
                         struct argp_state *state) {
  ieee_context_t *ctx = (ieee_context_t *)state->input;
  switch (key) {
  case KEY_DEBUG:
    ctx->debug = true;
    break;
  case KEY_DEBUG_BINARY:
    ctx->debug_binary = true;
    break;
  case KEY_NO_BACKEND_NAME:
    ctx->no_backend_name = true;
    break;
  case KEY_PRINT_NEW_LINE:
    ctx->print_new_line = true;
    break;
  case KEY_PRINT_SUBNORMAL_NORMALIZED:
    ctx->print_subnormal_normalized = true;
    break;
  case KEY_COUNT_OP:
    ctx->count_op = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void INTERFLOP_IEEE_API(finalize)(void *context) {
  ieee_context_t *my_context = (ieee_context_t *)context;

  if (my_context->count_op) {
    interflop_fprintf(stderr, "operations count:\n");
    interflop_fprintf(stderr, "\t mul=%ld\n", my_context->mul_count);
    interflop_fprintf(stderr, "\t div=%ld\n", my_context->div_count);
    interflop_fprintf(stderr, "\t add=%ld\n", my_context->add_count);
    interflop_fprintf(stderr, "\t sub=%ld\n", my_context->sub_count);
  };
}

static void init_context(ieee_context_t *context) {
  context->debug = false;
  context->debug_binary = false;
  context->no_backend_name = false;
  context->print_new_line = false;
  context->print_subnormal_normalized = false;
  context->count_op = false;
  context->mul_count = 0;
  context->div_count = 0;
  context->add_count = 0;
  context->sub_count = 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void INTERFLOP_IEEE_API(CLI)(int argc, char **argv, void *context) {
  /* parse backend arguments */
  ieee_context_t *ctx = (ieee_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic("Interflop backend error: argp_parse not implemented\n"
                    "Provide implementation or use interflop_configure to "
                    "configure the backend\n");
  }
}

#define CHECK_IMPL(name)                                                       \
  if (interflop_##name == Null) {                                              \
    interflop_panic("Interflop backend error: " #name " not implemented\n");   \
  }

void _ieee_check_stdlib(void) {
  CHECK_IMPL(malloc);
  CHECK_IMPL(exit);
  CHECK_IMPL(fopen);
  CHECK_IMPL(fprintf);
  CHECK_IMPL(getenv);
  CHECK_IMPL(gettid);
  CHECK_IMPL(sprintf);
  CHECK_IMPL(strcasecmp);
  CHECK_IMPL(strerror);
  CHECK_IMPL(vfprintf);
  CHECK_IMPL(vwarnx);
}

void INTERFLOP_IEEE_API(pre_init)(File *stream, interflop_panic_t panic,
                                  void **context) {
  interflop_set_handler("panic", panic);
  _ieee_check_stdlib();

  /* Initialize the logger */
  logger_init(stream);

  /* allocate the context */
  ieee_context_t *ctx =
      (ieee_context_t *)interflop_malloc(sizeof(ieee_context_t));
  init_context(ctx);
  *context = ctx;

  /* register %b format */
  register_printf_bit();
}

struct interflop_backend_interface_t INTERFLOP_IEEE_API(init)(void *context) {
  /* TODO: print_information_header*/

  struct interflop_backend_interface_t interflop_backend_ieee = {
    interflop_add_float : INTERFLOP_IEEE_API(add_float),
    interflop_sub_float : INTERFLOP_IEEE_API(sub_float),
    interflop_mul_float : INTERFLOP_IEEE_API(mul_float),
    interflop_div_float : INTERFLOP_IEEE_API(div_float),
    interflop_cmp_float : INTERFLOP_IEEE_API(cmp_float),
    interflop_add_double : INTERFLOP_IEEE_API(add_double),
    interflop_sub_double : INTERFLOP_IEEE_API(sub_double),
    interflop_mul_double : INTERFLOP_IEEE_API(mul_double),
    interflop_div_double : INTERFLOP_IEEE_API(div_double),
    interflop_cmp_double : INTERFLOP_IEEE_API(cmp_double),
    interflop_cast_double_to_float : INTERFLOP_IEEE_API(cast_double_to_float),
    interflop_fma_float : INTERFLOP_IEEE_API(fma_float),
    interflop_fma_double : INTERFLOP_IEEE_API(fma_double),
    interflop_enter_function : NULL,
    interflop_exit_function : NULL,
    interflop_user_call : NULL,
    interflop_finalize : INTERFLOP_IEEE_API(finalize)
  };

  return interflop_backend_ieee;
}

struct interflop_backend_interface_t interflop_init(void *context)
    __attribute__((weak, alias("interflop_ieee_init")));

void interflop_pre_init(File *stream, interflop_panic_t panic, void **context)
    __attribute__((weak, alias("interflop_ieee_pre_init")));

void interflop_CLI(int argc, char **argv, void *context)
    __attribute__((weak, alias("interflop_ieee_CLI")));
