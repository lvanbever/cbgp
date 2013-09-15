// ==================================================================
// @(#)as-level.h
//
// Provide structures and functions to handle an AS-level topology
// with business relationships. This is the foundation for handling
// AS-level topologies inferred by Subramanian et al and further by
// CAIDA.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: as-level.h,v 1.2 2009-06-25 14:31:03 bqu Exp $
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

#ifndef __BGP_ASLEVEL_H__
#define __BGP_ASLEVEL_H__

#include <net/prefix.h>
#include <bgp/aslevel/types.h>

// ----- Addressing schemes -----
#define ASLEVEL_ADDR_SCH_GLOBAL  0
#define ASLEVEL_ADDR_SCH_LOCAL   1
#define ASLEVEL_ADDR_SCH_DEFAULT ASLEVEL_ADDR_SCH_GLOBAL

// ----- File format -----
#define ASLEVEL_FORMAT_REXFORD 0
#define ASLEVEL_FORMAT_CAIDA   1
#define ASLEVEL_FORMAT_MEULLE  2
#define ASLEVEL_FORMAT_DEFAULT ASLEVEL_FORMAT_REXFORD

// ----- Topology state -----
#define ASLEVEL_STATE_LOADED    0
#define ASLEVEL_STATE_INSTALLED 1
#define ASLEVEL_STATE_POLICIES  2
#define ASLEVEL_STATE_RUNNING   3

// ----- Local preferences -----
#define ASLEVEL_PREF_PROV 60
#define ASLEVEL_PREF_PEER 80
#define ASLEVEL_PREF_CUST 100

// ----- Error codes -----
#define ASLEVEL_SUCCESS                 0
#define ASLEVEL_ERROR_UNEXPECTED        -1
#define ASLEVEL_ERROR_OPEN              -2
#define ASLEVEL_ERROR_NUM_PARAMS        -3
#define ASLEVEL_ERROR_INVALID_ASNUM     -4
#define ASLEVEL_ERROR_INVALID_RELATION  -5
#define ASLEVEL_ERROR_INVALID_DELAY     -6
#define ASLEVEL_ERROR_DUPLICATE_LINK    -7
#define ASLEVEL_ERROR_LOOP_LINK         -8
#define ASLEVEL_ERROR_NODE_EXISTS       -9
#define ASLEVEL_ERROR_NO_TOPOLOGY       -10
#define ASLEVEL_ERROR_TOPOLOGY_LOADED   -11
#define ASLEVEL_ERROR_UNKNOWN_FORMAT    -12
#define ASLEVEL_ERROR_UNKNOWN_ADDRSCH   -13
#define ASLEVEL_ERROR_CYCLE_DETECTED    -14
#define ASLEVEL_ERROR_DISCONNECTED      -15
#define ASLEVEL_ERROR_INCONSISTENT      -16
#define ASLEVEL_ERROR_UNKNOWN_FILTER    -17
#define ASLEVEL_ERROR_INVALID_STATE     -18
#define ASLEVEL_ERROR_NOT_INSTALLED     -19
#define ASLEVEL_ERROR_ALREADY_INSTALLED -20
#define ASLEVEL_ERROR_ALREADY_RUNNING   -21

// ----- Business relationships -----
#define ASLEVEL_PEER_TYPE_CUSTOMER 0
#define ASLEVEL_PEER_TYPE_PROVIDER 1
#define ASLEVEL_PEER_TYPE_PEER     2
#define ASLEVEL_PEER_TYPE_SIBLING  3
#define ASLEVEL_PEER_TYPE_UNKNOWN  255

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_perror ]-----------------------------------------
  void aslevel_perror(gds_stream_t * stream, int error);
  // -----[ aslevel_strerror ]---------------------------------------
  char * aslevel_strerror(int error);


  ///////////////////////////////////////////////////////////////////
  // TOPOLOGY MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_topo_create ]------------------------------------
  as_level_topo_t * aslevel_topo_create(uint8_t addr_scheme);
  // -----[ aslevel_topo_destroy ]-----------------------------------
  void aslevel_topo_destroy(as_level_topo_t ** topo);
  // -----[ aslevel_topo_add_as ]------------------------------------
  as_level_domain_t * aslevel_topo_add_as(as_level_topo_t * topo,
					  uint16_t asn);

  // -----[ aslevel_topo_remove_as ]---------------------------------
  /** Remove an ASN from the topology.
   *
   * @param topo is the AS-level topology.
   * @param asn  is the ASN of the AS to be removed.
   * @return 0 in case of success, <0 in case of error.
   */
  int aslevel_topo_remove_as(as_level_topo_t * topo, asn_t asn);

  // -----[ aslevel_top_get_as ]-------------------------------------
  as_level_domain_t * aslevel_topo_get_as(as_level_topo_t * topo,
					  uint16_t asn);
  // -----[ aslevel_as_num_providers ]-------------------------------
  unsigned int aslevel_as_num_providers(as_level_domain_t * domain);
  // -----[ aslevel_as_add_link ]------------------------------------
  int aslevel_as_add_link(as_level_domain_t * domain1,
			  as_level_domain_t * domain2,
			  peer_type_t peer_type,
			  as_level_link_t ** link);
  // -----[ aslevel_as_get_link ]------------------------------------
  as_level_link_t * aslevel_as_get_link(as_level_domain_t * domain1,
				      as_level_domain_t * domain2);
  // -----[ aslevel_as_get_link_by_index ]-----------------------------
  as_level_link_t * aslevel_as_get_link_by_index(as_level_domain_t * domain,
						 int index);
  // -----[ aslevel_as_get_num_links ]-------------------------------
  unsigned int aslevel_as_get_num_links(as_level_domain_t * domain);

  // -----[ aslevel_link_get_peer_type ]-----------------------------
  peer_type_t aslevel_link_get_peer_type(as_level_link_t * link);

  // -----[ aslevel_topo_check_cycle ]-------------------------------
  int aslevel_topo_check_cycle(as_level_topo_t * topo, int verbose);
  // -----[ aslevel_topo_check_consistency ]-------------------------
  int aslevel_topo_check_consistency(as_level_topo_t * topo);

  // -----[ aslevel_topo_build_network ]-----------------------------
  int aslevel_topo_build_network(as_level_topo_t * topo);

  // -----[ aslevel_topo_info ]--------------------------------------
  int aslevel_topo_info(gds_stream_t * stream);
  // -----[ aslevel_topo_num_nodes ]---------------------------------
  unsigned int aslevel_topo_num_nodes(as_level_topo_t * topo);
  // -----[ aslevel_as_num_customers ]-------------------------------
  unsigned int aslevel_as_num_customers(as_level_domain_t * fomain);
  // -----[ aslevel_as_num_providers ]-------------------------------
  unsigned int aslevel_as_num_providers(as_level_domain_t * domain);


  ///////////////////////////////////////////////////////////////////
  // TOPOLOGY API
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_topo_str2format ]--------------------------------
  int aslevel_topo_str2format(const char * str, uint8_t * format);

  // -----[ aslevel_topo_load ]--------------------------------------
  int aslevel_topo_load(const char * filename, uint8_t format,
			uint8_t addr_scheme, int * line_number);
  // -----[ aslevel_topo_check ]-------------------------------------
  int aslevel_topo_check(int verbose);
  // -----[ aslevel_topo_dump ]--------------------------------------
  int aslevel_topo_dump(gds_stream_t * stream, int verbose);
  // -----[ aslevel_topo_dump_graphviz ]-----------------------------
  int aslevel_topo_dump_graphviz(gds_stream_t * stream);
  // -----[ aslevel_topo_filter_as ]---------------------------------
  int aslevel_topo_filter_as(asn_t asn);
  // -----[ aslevel_topo_filter_group ]------------------------------
  int aslevel_topo_filter_group(uint8_t filter);
  // -----[ aslevel_topo_install ]-----------------------------------
  int aslevel_topo_install();
  // -----[ aslevel_topo_policies ]----------------------------------
  int aslevel_topo_policies();
  // -----[ aslevel_topo_run ]---------------------------------------
  int aslevel_topo_run();

  // -----[ aslevel_get_topo ]---------------------------------------
  as_level_topo_t * aslevel_get_topo();

  // -----[ aslevel_topo_dump_stubs ]--------------------------------
  int aslevel_topo_dump_stubs(gds_stream_t * stream);
  // -----[ aslevel_topo_record_route ]--------------------------------
  int aslevel_topo_record_route(gds_stream_t * stream, ip_pfx_t prefix,
				uint8_t options);

  ///////////////////////////////////////////////////////////////////
  // FILTERS MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_str2peer_type ]----------------------------------
  peer_type_t aslevel_str2peer_type(const char * pcStr);
  // -----[ aslevel_peer_type2str ]----------------------------------
  const char * aslevel_peer_type2str(peer_type_t peer_type);
  // -----[ aslevel_reverse_relation ]-------------------------------
  peer_type_t aslevel_reverse_relation(peer_type_t peer_type);
  // -----[ aslevel_filter_in ]--------------------------------------
  bgp_filter_t * aslevel_filter_in(peer_type_t peer_type);
  // -----[ aslevel_filter_out ]-------------------------------------
  bgp_filter_t * aslevel_filter_out(peer_type_t peer_type);


  ///////////////////////////////////////////////////////////////////
  // ADDRESSING SCHEMES MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ aslevel_str2addr_sch ]-----------------------------------
  int aslevel_str2addr_sch(const char * str, uint8_t * addr_scheme);
  // -----[ aslevel_addr_sch_default_get ]---------------------------
  net_addr_t aslevel_addr_sch_default_get(uint16_t asn);
  // -----[ aslevel_addr_sch_local_get ]-----------------------------
  net_addr_t aslevel_addr_sch_local_get(uint16_t asn);


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION & FINALIZATION
  ///////////////////////////////////////////////////////////////////

  // -----[ _aslevel_destroy ]---------------------------------------
  void _aslevel_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_H__ */
