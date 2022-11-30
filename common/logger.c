/*****************************************************************************\
 *                                                                           *\
 *  This File is part of the Verificarlo project,                            *\
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
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
// #include <err.h>
// #include <errno.h>
// #include <locale.h>
#include <stdarg.h>
// #include <stdbool.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <strings.h>
// #include <sys/syscall.h>
// #include <sys/types.h>
// #include <unistd.h>

#include "../interflop-stdlib/interflop_stdlib.h"

#define str(X) #X
#define xstr(X) str(X)

#ifndef BACKEND_HEADER
#define BACKEND_HEADER ""
#endif

#define BACKEND_HEADER_STR xstr(BACKEND_HEADER)

static const char backend_header[] = BACKEND_HEADER_STR;

/* ANSI colors */
typedef enum {
  red = 0,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  bold_red,
  bold_green,
  bold_yellow,
  bold_blue,
  bold_magenta,
  bold_cyan,
  reset
} ansi_colors_t;

/* ANSI escape sequences for normal colors */
static const char ansi_color_red[] = "\x1b[31m";
static const char ansi_color_green[] = "\x1b[32m";
static const char ansi_color_yellow[] = "\x1b[33m";
static const char ansi_color_blue[] = "\x1b[34m";
static const char ansi_color_magenta[] = "\x1b[35m";
static const char ansi_color_cyan[] = "\x1b[36m";
static const char ansi_color_reset[] = "\x1b[0m";

/* ANSI escape sequences for bold colors */
static const char ansi_color_bold_red[] = "\x1b[1;31m";
static const char ansi_color_bold_green[] = "\x1b[1;32m";
static const char ansi_color_bold_yellow[] = "\x1b[1;33m";
static const char ansi_color_bold_blue[] = "\x1b[1;34m";
static const char ansi_color_bold_magenta[] = "\x1b[1;35m";
static const char ansi_color_bold_cyan[] = "\x1b[1;36m";

/* Array of ANSI colors */
/* The order of the colors must be the same than the ansi_colors_t one */
static const char *ansi_colors[] = {
    ansi_color_red,       ansi_color_green,        ansi_color_yellow,
    ansi_color_blue,      ansi_color_magenta,      ansi_color_cyan,
    ansi_color_bold_red,  ansi_color_bold_green,   ansi_color_bold_yellow,
    ansi_color_bold_blue, ansi_color_bold_magenta, ansi_color_bold_cyan,
    ansi_color_reset};

/* Define the color of each level */
typedef enum {
  backend_color = green,
  info_color = bold_blue,
  warning_color = bold_yellow,
  error_color = bold_red,
  reset_color = reset
} level_color;

/* Environment variable for enabling/disabling the logger */
static const char vfc_backends_logger[] = "VFC_BACKENDS_LOGGER";

/* Environment variable for specifying the verificarlo logger output File */
static const char vfc_backends_logfile[] = "VFC_BACKENDS_LOGFILE";

/* Environment variable for enabling/disabling the color */
static const char vfc_backends_colored_logger[] = "VFC_BACKENDS_COLORED_LOGGER";

static IBool logger_enabled = ITrue;
static IBool logger_colored = IFalse;
static File *logger_logfile = Null;

/* Returns ITrue if the logger is enabled */
IBool is_logger_enabled(void) {
  const char *is_logger_enabled_env = interflop_getenv(vfc_backends_logger);
  if (is_logger_enabled_env == Null) {
    return ITrue;
  } else if (interflop_strcasecmp(is_logger_enabled_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

/* Returns ITrue if the color is enabled */
IBool is_logger_colored(void) {
  const char *is_colored_logger_env =
      interflop_getenv(vfc_backends_colored_logger);
  if (is_colored_logger_env == Null) {
    return IFalse;
  } else if (interflop_strcasecmp(is_colored_logger_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

static void _interflop_err(int eval, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  interflop_vwarnx(fmt, ap);
  va_end(ap);
  interflop_exit(eval);
}

static void _interflop_verrx(int eval, const char *fmt, va_list args) {
  interflop_vwarnx(fmt, args);
  interflop_exit(eval);
}

void set_logger_logfile(File *stream) {
  if (logger_logfile != Null) {
    return;
  }
  const char *logger_logfile_env = interflop_getenv(vfc_backends_logfile);
  if (logger_logfile_env == Null) {
    logger_logfile = stream;
  } else {
    /* Create log File specific to TID to avoid non-deterministic output */
    char tmp[1024];
    int r =
        interflop_sprintf(tmp, "%s.%d", logger_logfile_env, interflop_gettid());
    if (r < 0) {
      _interflop_err(EXIT_FAILURE, "Error while creating %s name\n",
                     logger_logfile_env);
    }
    int error = 0;
    logger_logfile = interflop_fopen(tmp, "a", &error);
    if (logger_logfile == Null) {
      char *msg = interflop_strerror(error);
      _interflop_err(EXIT_FAILURE, "Error [%s]: %s", BACKEND_HEADER_STR, msg);
    }
  }
}

void logger_header(File *stream, const char *lvl_name,
                   const level_color lvl_color, const IBool colored) {
  if (colored) {
    interflop_fprintf(stream, "%s%s%s [%s%s%s]: ", ansi_colors[lvl_color],
                      lvl_name, ansi_colors[reset_color],
                      ansi_colors[backend_color], BACKEND_HEADER_STR,
                      ansi_colors[reset_color]);
  } else {
    interflop_fprintf(stream, "%s [%s]: ", lvl_name, backend_header);
  }
}

/* Display the info message */
void logger_info(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    va_list ap;
    va_start(ap, fmt);
    interflop_vfprintf(logger_logfile, fmt, ap);
    va_end(ap);
  }
}

/* Display the warning message */
void logger_warning(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Warning", warning_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  interflop_vwarnx(fmt, ap);
  va_end(ap);
}

/* Display the error message */
void logger_error(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Error", error_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  _interflop_verrx(EXIT_FAILURE, fmt, ap);
  va_end(ap);
}

/* Display the info message */
void vlogger_info(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    interflop_vfprintf(logger_logfile, fmt, argp);
  }
}

/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Warning", warning_color, logger_colored);
  }
  interflop_vwarnx(fmt, argp);
}

/* Display the error message */
void vlogger_error(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Error", error_color, logger_colored);
  }
  _interflop_verrx(EXIT_FAILURE, fmt, argp);
}

void logger_init(File *stream) {
  logger_enabled = is_logger_enabled();
  logger_colored = is_logger_colored();
  set_logger_logfile(stream);
}
