// ==================================================================
// @(#)as-level-util.h
//
// Provides utility functions to manage and handle large AS-level
// topologies.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: util.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
//
// C-BGP, BGP Routing Solver
// Copyright (C) 2002-2008 Bruno Quoitin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307  USA
// ==================================================================

#ifndef __BGP_ASLEVEL_UTIL_H__
#define __BGP_ASLEVEL_UTIL_H__

#include <bgp/as.h>

#define AS_SET_SIZE MAX_AS/(sizeof(unsigned int)*8)
#define AS_SET_DEFINE(SET) unsigned int SET[AS_SET_SIZE]
#define AS_SET_INIT(SET) memset(SET, 0, sizeof(SET));
#define IS_IN_AS_SET(SET,ASN) ((SET[ASN / (sizeof(unsigned int)*8)] &\
				(1 << (ASN % (sizeof(unsigned int)*8)))) != 0)
#define AS_SET_PUT(SET,ASN) SET[ASN / (sizeof(unsigned int)*8)]|=\
  1 << (ASN % (sizeof(unsigned int)*8));
#define AS_SET_CLEAR(SET,ASN) SET[ASN / (sizeof(unsigned int)*8)]&= \
  ~(1 << (ASN % (sizeof(unsigned int)*8)));

#endif /* __BGP_ASLEVEL_UTIL_H__ */
