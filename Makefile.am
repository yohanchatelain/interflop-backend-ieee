lib_LTLIBRARIES = libinterflop_ieee.la
libinterflop_ieee_la_SOURCES = interflop_ieee.c common/printf_specifier.c common/logger.c
libinterflop_ieee_la_CFLAGS = -DBACKEND_HEADER="interflop_ieee" -O3 -I@INTERFLOP_STDLIB_PATH@/include/
if WALL_CFLAGS
libinterflop_ieee_la_CFLAGS += -Wall -Wextra
endif
libinterflop_ieee_la_LDFLAGS = -I@INTERFLOP_STDLIB_PATH@/lib/libinterlop_fma.la
library_includedir =$(includedir)/
