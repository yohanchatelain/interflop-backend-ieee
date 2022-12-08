lib_LTLIBRARIES = libinterflop_ieee.la
libinterflop_ieee_la_SOURCES = \
    interflop_ieee.c \
    common/printf_specifier.c \
    @INTERFLOP_STDLIB_PATH@/include/interflop-stdlib/iostream/logger.c
libinterflop_ieee_la_CFLAGS = \
    -DBACKEND_HEADER="interflop_ieee" \
    -I@INTERFLOP_STDLIB_PATH@/include/ \
    -fno-stack-protector -O3
if WALL_CFLAGS
libinterflop_ieee_la_CFLAGS += -Wall -Wextra -g
endif
libinterflop_ieee_la_LIBADD = \
     @INTERFLOP_STDLIB_PATH@/lib/libinterflop_fma.la
if LINK_INTERFLOP_STDLIB
libinterflop_ieee_la_LIBADD += @INTERFLOP_STDLIB_PATH@/lib/libinterflop_stdlib.la
endif
library_includedir =$(includedir)/
include_HEADERS = interflop_ieee.h