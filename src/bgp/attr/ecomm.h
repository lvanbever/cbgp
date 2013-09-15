// ==================================================================
// @(#)ecomm.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/12/2002
// $Id: ecomm.h,v 1.1 2009-03-24 13:41:26 bqu Exp $
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
 * Provide functions to manage BGP Extended Communities attributes.
 */

#ifndef __ECOMM_H__
#define __ECOMM_H__

#include <libgds/stream.h>
#include <libgds/types.h>

#include <bgp/attr/types.h>
#include <bgp/types.h>
#include <net/prefix.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ecomm_val_destroy ----------------------------------------
  void ecomm_val_destroy(bgp_ecomm_t ** comm_ref);
  // ----- ecomm_val_copy -------------------------------------------
  bgp_ecomm_t * ecomm_val_copy(bgp_ecomm_t * comm);

  // -----[ ecomms_create ]------------------------------------------
  /**
   * Create an Extended Communities attribute.
   */
  bgp_ecomms_t * ecomms_create();

  // -----[ ecomms_destroy ]-----------------------------------------
  /**
   * Destroy an Extended Communities attribute.
   */
  void ecomms_destroy(bgp_ecomms_t ** comms_ref);

  // -----[ ecomms_length ]------------------------------------------
  /**
   * Return the number of values in an Extended Communities
   * attribute.
   */
  unsigned char ecomms_length(bgp_ecomms_t * comms);

  // -----[ ecomms_get_at ]------------------------------------------
  /**
   * Get a value from an Extended Communities attribute.
   */
  bgp_ecomm_t * ecomms_get_at(bgp_ecomms_t * comms,
			      unsigned int index);

  // -----[ ecomms_dup ]---------------------------------------------
  /**
   * Duplicate an Extended Communities attribute.
   */
  bgp_ecomms_t * ecomms_dup(bgp_ecomms_t * comms);

  // -----[ ecomms_add ]---------------------------------------------
  /**
   * Add a value to an Extended Communities attribute.
   */
  int ecomms_add(bgp_ecomms_t ** comms_ref, bgp_ecomm_t * comm);

  // -----[ ecomms_strip_non_transitive ]----------------------------
  /**
   * Remove all the non-transitive values from an Extended
   * Communities attribute.
   */
  void ecomms_strip_non_transitive(bgp_ecomms_t ** comms_ref);

  // -----[ ecomms_dump ]--------------------------------------------
  void ecomm_dump(gds_stream_t * stream, bgp_ecomms_t * comms,
		  int text);

  // -----[ ecomms_equals ]------------------------------------------
  int ecomm_equals(bgp_ecomms_t * comms1,
		   bgp_ecomms_t * comms2);

  // -----[ ecomm_red_create ]---------------------------------------
  bgp_ecomm_t * ecomm_red_create_as(unsigned char action_type,
				    unsigned char action_param,
				    asn_t asn);
  // -----[ ecomm_red_match ]----------------------------------------
  int ecomm_red_match(bgp_ecomm_t * comm, bgp_peer_t * peer);

  // -----[ ecomm_red_dump ]-----------------------------------------
  void ecomm_red_dump(gds_stream_t * stream, bgp_ecomm_t * comm);

  // -----[ ecomm_val_dump ]-----------------------------------------
  void ecomm_val_dump(gds_stream_t * stream, bgp_ecomm_t * comm,
		      int text);
  
#ifdef __EXPERIMENTAL__
  // -----[ ecomm_depref_create ]------------------------------------
  bgp_ecomm_t * ecomm_pref_create(uint32_t pref);
  // -----[ ecomm_pref_dump ]----------------------------------------
  void ecomm_pref_dump(gds_stream_t * stream, bgp_ecomm_t * comm);
  // -----[ ecomm_pref_get ]-----------------------------------------
  uint32_t ecomm_pref_get(bgp_ecomm_t * comm);
#endif /* __EXPERIMENTAL__ */

#ifdef __cplusplus
}
#endif

#endif
