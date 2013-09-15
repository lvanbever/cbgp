// ==================================================================
// @(#)icmp_options.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 01/0/2008
// $Id: icmp_options.h,v 1.4 2009-08-31 09:48:28 bqu Exp $
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

#ifndef __NET_IP_OPTIONS_H__
#define __NET_IP_OPTIONS_H__

#include <assert.h>

#include <libgds/array.h>
#include <libgds/fifo.h>
#include <libgds/stack.h>

#include <net/error.h>
#include <net/ip_trace.h>
#include <net/net_types.h>
#include <net/routing.h>

#define IP_OPT_DELAY      0x0001 /* Record total delay */
#define IP_OPT_WEIGHT     0x0002 /* Record total weight */
#define IP_OPT_CAPACITY   0x0004 /* Record max capacity */
#define IP_OPT_LOAD       0x0008 /* Load traversed interfaces */
#define IP_OPT_LOOPBACK   0x0010
#define IP_OPT_DEFLECTION 0x0020 /* Check for deflection */
#define IP_OPT_QUICK_LOOP 0x0040 /* Check for forwarding loops */
#define IP_OPT_ALT_DEST   0x0080 /* Use alternate destination (prefix) */
#define IP_OPT_ECMP       0x0100 /* Enumerate equal-cost paths */
#define IP_OPT_TUNNEL     0x0200 /* Show path inside tunnels */
#define IP_OPT_TRACE      0x0400 /* Record trace */

typedef struct ip_opt_t {
  uint16_t          flags;
  net_link_load_t   load;      /* Amount of traffic to load accross path */
  ip_pfx_t          alt_dest;  /* Alternate destination for lookup */
  unsigned int      ref_cnt;
  ip_trace_t      * trace;
  gds_fifo_t      * fifo_trace;
} ip_opt_t;

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // IP OPTIONS SETUP
  ///////////////////////////////////////////////////////////////////

  // -----[ ip_options_init ]----------------------------------------
  void ip_options_init(ip_opt_t * opts);
  // -----[ ip_options_create ]--------------------------------------
  ip_opt_t * ip_options_create(void);
  // -----[ ip_options_destroy ]-------------------------------------
  void ip_options_destroy(ip_opt_t ** opts_ref);
  // -----[ ip_options_copy ]----------------------------------------
  ip_opt_t * ip_options_copy(ip_opt_t * opts, int with_trace);
  // -----[ ip_options_add_ref ]-------------------------------------
  void ip_options_add_ref(ip_opt_t * opts);
  // -----[ ip_options_set ]-----------------------------------------
  void ip_options_set(ip_opt_t * opts, uint16_t flag);
  // -----[ ip_options_load ]----------------------------------------
  void ip_options_load(ip_opt_t * opts, net_link_load_t load);
  // -----[ ip_options_alt_dest ]------------------------------------
  void ip_options_alt_dest(ip_opt_t * opts, ip_pfx_t alt_dest);
  // -----[ ip_options_trace ]---------------------------------------
  void ip_options_trace(ip_opt_t * opts);


  ///////////////////////////////////////////////////////////////////
  // IP OPTIONS PROCESSING
  ///////////////////////////////////////////////////////////////////

  // -----[ ip_opt_hook_msg_error ]----------------------------------
  net_error_t ip_opt_hook_msg_error(net_msg_t * msg,
				    net_error_t error);
  // -----[ ip_opt_hook_msg_sent ]-----------------------------------
  net_error_t ip_opt_hook_msg_sent(net_node_t * node,
				   net_msg_t * msg,
				   const rt_entries_t ** rtentries);
  // -----[ ip_opt_hook_msg_rcvd ]-----------------------------------
  net_error_t ip_opt_hook_msg_rcvd(net_node_t * node,
				   net_iface_t * iif,
				   net_msg_t * msg);
  // -----[ ip_opt_hook_msg_in ]-------------------------------------
  net_error_t ip_opt_hook_msg_in(net_node_t * node,
				 net_iface_t * iif,
				 net_msg_t * msg,
				 const rt_entries_t ** rtentries);
  // -----[ ip_opt_hook_msg_out ]------------------------------------
  net_error_t ip_opt_hook_msg_out(net_node_t * node,
				  net_iface_t * oif,
				  net_msg_t * msg);
  // -----[ ip_opt_hook_msg_subnet ]---------------------------------
  net_error_t ip_opt_hook_msg_subnet(net_subnet_t * subnet,
				     net_msg_t * msg,
				     int * reached);
  // -----[ ip_opt_hook_msg_encap ]----------------------------------
  net_error_t ip_opt_hook_msg_encap(net_node_t * node,
				    net_msg_t * outer_msg,
				    net_msg_t * inner_msg);
  // -----[ ip_opt_hook_msg_decap ]----------------------------------
  net_error_t ip_opt_hook_msg_decap(net_node_t * node,
				    net_msg_t * outer_msg,
				    net_msg_t * inner_msg);
  // -----[ ip_opt_hook_msg_ecmp ]-----------------------------------
  net_error_t ip_opt_hook_msg_ecmp(net_node_t * node,
				   net_msg_t * msg,
				   const rt_entries_t ** rtentries);

  // -----[ ip_opt_ecmp_push ]---------------------------------------
  void ip_opt_ecmp_push(ip_opt_t * opts, net_node_t * node,
			net_msg_t * msg, rt_entry_t * rtentry);
  // -----[ ip_opt_ecmp_has_next ]-----------------------------------
  int ip_opt_ecmp_has_next(ip_opt_t * opts);
  // -----[ ip_opt_ecmp_get_next ]-----------------------------------
  ip_trace_t ** ip_opt_ecmp_get_next(ip_opt_t * opts);
  
  array_t * ip_opt_ecmp_run(ip_opt_t * opts, net_msg_t * init_msg, net_node_t * node);

#ifdef __cplusplus
}
#endif

#endif /* __NET_IP_OPTIONS_H__ */

