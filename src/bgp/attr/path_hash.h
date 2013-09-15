// ==================================================================
// @(#)path_hash.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/10/2005
// $Id: path_hash.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

#ifndef __BGP_PATH_HASH_H__
#define __BGP_PATH_HASH_H__

#include <libgds/stream.h>
#include <stdlib.h>

#define PATH_HASH_METHOD_STRING 0
#define PATH_HASH_METHOD_ZEBRA  1
#define PATH_HASH_METHOD_OAT    2

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ path_hash_add ]------------------------------------------
  void * path_hash_add(bgp_path_t * path);
  // -----[ path_hash_get ]------------------------------------------
  bgp_path_t * path_hash_get(bgp_path_t * path);
  // -----[ path_hash_remove ]---------------------------------------
  int path_hash_remove(bgp_path_t * path);
  // -----[ path_hash_get_size ]-------------------------------------
  unsigned int path_hash_get_size();
  // -----[ path_hash_set_size ]-------------------------------------
  int path_hash_set_size(unsigned int size);
  // -----[ path_hash_get_method ]-----------------------------------
  uint8_t path_hash_get_method();
  // -----[ path_hash_set_method ]-----------------------------------
  int path_hash_set_method(uint8_t method);
  
  // -----[ path_hash_content ]--------------------------------------
  void path_hash_content(gds_stream_t * stream);
  // -----[ path_hash_statistics ]-----------------------------------
  void path_hash_statistics(gds_stream_t * stream);
  
  // -----[ _path_hash_init ]----------------------------------------
  void _path_hash_init();
  // -----[ _path_hash_destroy ]-------------------------------------
  void _path_hash_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_HASH_H__ */
