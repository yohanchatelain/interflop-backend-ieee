/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2019-2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#ifndef __INTERFLOP_IEEE_H__
#define __INTERFLOP_IEEE_H__

#include "../../../interflop-stdlib/interflop_stdlib.h"

#define INTERFLOP_IEEE_API(name) interflop_ieee_##name

/* Interflop context */
typedef struct {
  unsigned long int mul_count;
  unsigned long int div_count;
  unsigned long int add_count;
  unsigned long int sub_count;
  IBool debug;
  IBool debug_binary;
  IBool no_backend_name;
  IBool print_new_line;
  IBool print_subnormal_normalized;
  IBool count_op;
} ieee_context_t;

#endif /* __INTERFLOP_IEEE_H__ */