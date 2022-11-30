noinst_LTLIBRARIES = libinterflop_fma.la

libinterflop_fma_la_CFLAGS = -fPIC -fno-stack-protector
if WALL_CFLAGS
libinterflop_fma_la_CFLAGS += -Wall -Wextra 
endif
libinterflop_fma_la_SOURCES = \
    fmaqApproc.h \
    fmaqApprox.c 
library_includedir = $(includedir)/
