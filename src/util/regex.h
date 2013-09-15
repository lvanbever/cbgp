// ==================================================================
// @(#)regex.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/09/2004
// $Id: regex.h,v 1.4 2009-03-24 16:30:03 bqu Exp $
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

#ifndef __UTIL_REGEX_H__
#define __UTIL_REGEX_H__

#include <libgds/types.h>

typedef struct RegEx SRegEx;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- regex_int ------------------------------------------------
  SRegEx * regex_init(const char * pattern, const unsigned int uMaxResult);
  // ----- regex_reinit ---------------------------------------------
  void regex_reinit(SRegEx * pRegEx);
  // ----- regex_search ---------------------------------------------
  int regex_search(SRegEx * pRegEx, const char * sExp);
  // ----- regex_get_result -----------------------------------------
  char * regex_get_result(SRegEx * pRegEx, const char * sExp);
  // ----- regex_finalize -------------------------------------------
  void regex_finalize(SRegEx ** pRegEx);

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_REGEX_H__ */
