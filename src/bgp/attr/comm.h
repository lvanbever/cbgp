// ==================================================================
// @(#)comm.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/05/2003
// $Id: comm.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

/**
 * \file
 * Provide functions to manage BGP Communities attributes.
 */

#ifndef __BGP_COMM_H__
#define __BGP_COMM_H__

#include <libgds/stream.h>

#include <bgp/attr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ comms_create ]-------------------------------------------
  /**
   * Create a Communities attribute.
   */
  bgp_comms_t * comms_create();

  // -----[ comms_destroy ]------------------------------------------
  /**
   * Destroy a Communities attribute.
   */
  void comms_destroy(bgp_comms_t ** comms_ref);

  // -----[ comms_dup ]----------------------------------------------
  /**
   * Duplicate a Communities attribute.
   */
  bgp_comms_t * comms_dup(const bgp_comms_t * comms);

  // -----[ comms_length ]-------------------------------------------
  /**
   * Return the number of values in a Communities attribute.
   */
  static inline unsigned int comms_length(const bgp_comms_t * comms) {
    if (comms == NULL)
      return 0;
    return comms->num;
  }

  // -----[ comms_add ]----------------------------------------------
  /**
   * Add a value to a Communities attribute.
   */
  int comms_add(bgp_comms_t ** comms_ref, bgp_comm_t comm);

  // -----[ comms_remove ]-------------------------------------------
  /**
   * Remove a value from a Communities attribute.
   */
  void comms_remove(bgp_comms_t ** comms_ref, bgp_comm_t comm);

  // -----[ comms_contains ]-----------------------------------------
  /**
   * Test if a value is contained in a Communities attribute.
   */
  int comms_contains(bgp_comms_t * comms, bgp_comm_t comm);

  // -----[ comms_equals ]-------------------------------------------
  /**
   * Test if two Communities attributes are equal.
   */
  int comms_equals(const bgp_comms_t * comms1,
		   const bgp_comms_t * comms2);

  // -----[ comm_value_from_string ]---------------------------------
  /**
   *
   */
  int comm_value_from_string(const char * comm_str, bgp_comm_t * comm);

  // -----[ comm_value_dump ]----------------------------------------
  void comm_value_dump(gds_stream_t * stream, bgp_comm_t comm,
		       int as_text);

  // ----- comm_dump ------------------------------------------------
  void comm_dump(gds_stream_t * stream, const bgp_comms_t * comms,
		 int as_text);

  // -----[ comm_to_string ]-----------------------------------------
  int comm_to_string(bgp_comms_t * comms, char * buf, size_t buf_size);

  // -----[ comm_from_string ]---------------------------------------
  bgp_comms_t * comm_from_string(const char * comms_str);

  // -----[ comms_cmp ]----------------------------------------------
  /**
   * Compare two Communities attributes.
   */
  int comms_cmp(const bgp_comms_t * comms1, const bgp_comms_t * comms2);

  // -----[ _comm_destroy ]------------------------------------------
  /**
   * \internal
   * This function must be called by the CBGP library finalization
   * function.
   */
  void _comm_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_COMM_H__ */
