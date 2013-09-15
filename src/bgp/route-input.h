// ==================================================================
// @(#)route-input.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/05/2007
// $Id: route-input.h,v 1.5 2009-04-02 19:15:00 bqu Exp $
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

#ifndef __BGP_ROUTE_INPUT_H__
#define __BGP_ROUTE_INPUT_H__

#include <net/prefix.h>
#include <bgp/types.h>

// ----- BGP Routes Input Formats -----
typedef enum {
  BGP_ROUTES_INPUT_MRT_ASC,
#ifdef HAVE_BGPDUMP
  BGP_ROUTES_INPUT_MRT_BIN,
#endif /* HAVE_BGPDUMP */
  BGP_ROUTES_INPUT_CISCO,
  BGP_ROUTES_INPUT_MAX
} bgp_input_type_t;

// ----- Error codes -----
typedef enum {
  BGP_INPUT_SUCCESS         =  0,
  BGP_INPUT_ERROR_UNEXPECTED= -1,
  BGP_INPUT_ERROR_FILE_OPEN = -2,
  BGP_INPUT_ERROR_IGNORED   = -3,
  BGP_INPUT_ERROR_FILTERED  = -4,
  BGP_INPUT_ERROR_USER      = -5,
} bgp_input_error_t;

// ----- Status codes -----
#define BGP_INPUT_STATUS_OK       0
#define BGP_INPUT_STATUS_FILTERED 1
#define BGP_INPUT_STATUS_IGNORED  2

// -----[ bgp_route_handler_f ]--------------------------------------
typedef int (*bgp_route_handler_f)(int status,
				   bgp_route_t * route, net_addr_t peer_addr,
				   asn_t peer_asn, void * ctx);

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_routes_str2format ]----------------------------------
  int bgp_routes_str2format(const char * format_str,
			    bgp_input_type_t * format);
  // -----[ bgp_routes_load ]----------------------------------------
  int bgp_routes_load(const char * filename,
		      bgp_input_type_t format,
		      bgp_route_handler_f handler, void * ctx);
  // -----[ bgp_routes_load_list ]-----------------------------------
  bgp_routes_t * bgp_routes_load_list(const char * filename,
				      bgp_input_type_t format);
  // -----[ bgp_input_strerror ]-------------------------------------
  const char * bgp_input_strerror(bgp_input_error_t error);
  // -----[ bgp_input_perror ]---------------------------------------
  void bgp_input_perror(gds_stream_t * stream,
			bgp_input_error_t error);
  // -----[ bgp_input_set_user_error ]-------------------------------
  void bgp_input_set_user_error(const char * format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_INPUT_H__ */
