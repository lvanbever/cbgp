// ==================================================================
// @(#)path.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// $Id: path.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

#ifndef __BGP_PATH_H__
#define __BGP_PATH_H__

#include <stdio.h>

#include <libgds/stream.h>
#include <libgds/types.h>

#include <bgp/types.h>
#include <util/regex.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- path_create ----------------------------------------------
  bgp_path_t * path_create();
  // ----- path_destroy ---------------------------------------------
  void path_destroy(bgp_path_t ** path);
  // ----- path_max_value -------------------------------------------
  bgp_path_t * path_max_value();
  // ----- path_copy ------------------------------------------------
  bgp_path_t * path_copy(bgp_path_t * path);
  // ----- path_num_segments ----------------------------------------
  int path_num_segments(const bgp_path_t * path);
  // ----- path_length ----------------------------------------------
  int path_length(bgp_path_t * path);
  // ----- path_add_segment -----------------------------------------
  int path_add_segment(bgp_path_t * path, bgp_path_seg_t * segment);
  // ----- path_append ----------------------------------------------
  int path_append(bgp_path_t ** path, asn_t asn);
  // ----- path_contains --------------------------------------------
  int path_contains(bgp_path_t * path, asn_t asn);
  // -----[ path_last_as ]-------------------------------------------
  int path_last_as(bgp_path_t * path);
  // -----[ path_first_as ]------------------------------------------
  int path_first_as(bgp_path_t * path);
  // -----[ path_to_string ]-----------------------------------------
  int path_to_string(bgp_path_t * path, int reverse,
		     char * dst, size_t tDstSize);
  // -----[ path_from_string ]---------------------------------------
  bgp_path_t * path_from_string(const char * str);
  // ----- path_dump ------------------------------------------------
  void path_dump(gds_stream_t * stream, const bgp_path_t * path,
		 int reverse);
  // ----- path_hash ------------------------------------------------
  uint32_t path_hash(const bgp_path_t * path);
  // -----[ path_hash_zebra ]----------------------------------------
  uint32_t path_hash_zebra(const void * item, unsigned int hash_size);
  // -----[ path_hash_OAT ]--------------------------------------------
  uint32_t path_hash_OAT(const void * item, unsigned int hash_size);
  // ----- path_comparison ------------------------------------------
  int path_comparison(bgp_path_t * path1, bgp_path_t * path2);
  // ----- path_equals ----------------------------------------------
  int path_equals(const bgp_path_t * path1, const bgp_path_t * path2);
  // -----[ path_cmp ]-----------------------------------------------
  int path_cmp(const bgp_path_t * path1, const bgp_path_t * path2);
  // -----[ path_str_cmp ]-------------------------------------------
  int path_str_cmp(bgp_path_t * path1, bgp_path_t * path2);
  // ----- path_match -----------------------------------------------
  int path_match(bgp_path_t * path, SRegEx * pRegEx);
  // -----[ path_remove_private ]------------------------------------
  void path_remove_private(bgp_path_t * path);

  // -----[ _path_destroy ]------------------------------------------
  void _path_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_PATH_H__ */
