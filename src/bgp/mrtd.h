// ==================================================================
// @(#)mrtd.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// $Id: mrtd.h,v 1.10 2009-03-24 14:27:18 bqu Exp $
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

#ifndef __MRTD_H__
#define __MRTD_H__

#include <libgds/array.h>
#include <libgds/types.h>

#include <bgp/types.h>
#include <bgp/message.h>
#include <bgp/routes_list.h>
#include <bgp/route-input.h>
#include <util/lrp.h>

typedef uint8_t mrtd_input_t;

// ----- MRT file types -----
#define MRTD_TYPE_INVALID  0
#define MRTD_TYPE_RIB      'B' /* Best route */
#define MRTD_TYPE_UPDATE   'A' /* Advertisement */
#define MRTD_TYPE_WITHDRAW 'W' /* Withdraw */

// ----- MRT file formats -----
#define MRT_FORMAT_ASCII  0
#define MRT_FORMAT_BINARY 1

// -----[ mrtd_error_code_t ]----------------------------------------
typedef enum {
  MRTD_SUCCESS            = LRP_SUCCESS,
  MRTD_ERROR              = LRP_ERROR_USER,
  MRTD_ERROR_SYNTAX       = LRP_ERROR_USER-1,
  MRTD_INVALID_PEER_ADDR  = LRP_ERROR_USER-2,
  MRTD_INVALID_PEER_ASN   = LRP_ERROR_USER-3,
  MRTD_INVALID_ORIGIN     = LRP_ERROR_USER-4,
  MRTD_INVALID_ASPATH     = LRP_ERROR_USER-5,
  MRTD_INVALID_LOCALPREF  = LRP_ERROR_USER-6,
  MRTD_INVALID_MED        = LRP_ERROR_USER-7,
  MRTD_INVALID_COMMUNITIES= LRP_ERROR_USER-8,
  MRTD_INVALID_RECORD_TYPE= LRP_ERROR_USER-9,
  MRTD_INVALID_PREFIX     = LRP_ERROR_USER-10,
  MRTD_INVALID_NEXTHOP    = LRP_ERROR_USER-11,
  MRTD_MISSING_FIELDS     = LRP_ERROR_USER-12,
  MRTD_INVALID_PROTOCOL   = LRP_ERROR_USER-13,
} mrtd_error_code_t;

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // ASCII MRT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- mrtd_create_path -----------------------------------------
  bgp_path_t * mrtd_create_path(const char * path);
  // ----- mrtd_route_from_line -------------------------------------
  int mrtd_route_from_line(const char * line,
			   net_addr_t * peer_addr,
			   asn_t * peer_asn,
			   bgp_route_t ** route_ref);
  // ----- mrtd_msg_from_line ---------------------------------------
  int mrtd_msg_from_line(bgp_router_t * router, bgp_peer_t * peer,
			 const char * line, bgp_msg_t ** msg_ref);
  // ----- mrtd_ascii_load_routes -----------------------------------
  int mrtd_ascii_load(const char * filename, bgp_route_handler_f handler,
		      void * ctx);
  // -----[ mrtd_strerror ]------------------------------------------
  const char * mrtd_strerror(int result);
  // -----[ mrtd_perror ]--------------------------------------------
  void mrtd_perror(gds_stream_t * stream, int result);


  ///////////////////////////////////////////////////////////////////
  // BINARY MRT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

#ifdef HAVE_BGPDUMP
  // -----[ mrtd_binary_load ]---------------------------------------
  int mrtd_binary_load(const char * file_name, bgp_route_handler_f handler,
		       void * ctx);
#endif


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION & FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // ----- _mrtd_destroy --------------------------------------------
  void _mrtd_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __MRTD_H__ */
