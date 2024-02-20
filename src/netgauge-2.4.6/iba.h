/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */


// VERSION: 0.11

#ifndef H_IBA_H
#define H_IBA_H

#if defined IBA_USE_VAPI
#include "iba_vapi.h"
#elif defined IBA_USE_OPENIB
#include "iba_openib.h"
#else
#error NO INFINIBAND VERBS API SELECTED
#endif


#define SET(struct_type, struct_name, field_name, ...) (SET_##struct_type##__##field_name(struct_name, __VA_ARGS__))
#define GET(struct_type, struct_name, field_name, ...) (GET_##struct_type##__##field_name(struct_name, __VA_ARGS__))


#endif /* H_IBA_H */

