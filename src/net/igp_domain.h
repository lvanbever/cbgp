// ==================================================================
// @(#)domain.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/02/2002
// $Id: igp_domain.h,v 1.9 2009-03-24 16:12:49 bqu Exp $
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
 * Provide functions for handling IGP domains, i.e. set of routers
 * that are part of the same IGP domain.
 */

#ifndef __IGP_DOMAIN_H__
#define __IGP_DOMAIN_H__

#include <libgds/stream.h>
#include <libgds/radix-tree.h>
#include <net/net_types.h>
#include <net/network.h>

// -----[ FIGPDomainsForEach ]---------------------------------------
typedef int (*FIGPDomainsForEach)(igp_domain_t * domain, void * ctx);

#define ospf_domain_create(N) igp_domain_create(N, IGP_DOMAIN_OSPF)

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ igp_domain_create ]--------------------------------------
  /**
   * Create an IGP domain.
   *
   * \param id   is the IGP domain's identifier.
   * \param type is the IGP domain's type.
   * \retval an IGP domain.
   */
  igp_domain_t * igp_domain_create(uint16_t id, igp_domain_type_t type);

  // -----[ igp_domain_destroy ]-------------------------------------
  /**
   * Destroy a IGP domain.
   *
   * \param domain_ref is a reference to the IGP domain to be
   *                   destroyed.
   */
  void igp_domain_destroy(igp_domain_t ** domain_ref);

  // -----[ igp_domain_add_router ]----------------------------------
  /**
   * Add a router to an IGP domain.
   *
   * \param domain is the target IGP domain.
   * \param router is the new member router.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   *
   * Preconditions:
   * - the router must not exist in the domain. Otherwise, it will be
   *   replaced by the new one and memory leaks as well as unexpected
   *   results may occur.
   */
  int igp_domain_add_router(igp_domain_t * domain, net_node_t * router);

  // ----- igp_domain_contains_router -------------------------------
  /**
   * Test if a router belongs to an IGP domain.
   *
   * \param domain is the target IGP domain.
   * \param node   is the searched node.
   * \retval 1 in case the node belongs to the IGP domain,
   *         0 otherwize.
   */
  int igp_domain_contains_router(igp_domain_t * domain,
				 net_node_t * node);

  // -----[ igp_domain_contains_router_by_addr ]---------------------
  /**
   * Test if a router belongs to an IGP domain, based on the router's
   * RID.
   *
   * \param domain is the target IGP domain.
   * \param addr   is the searched node's RID.
   * \retval 1 in case the node belongs to the IGP domain,
   *         0 otherwize.
   */
  int igp_domain_contains_router_by_addr(igp_domain_t * domain,
					 net_addr_t addr);

  // ----- igp_domain_routers_for_each ------------------------------
  int igp_domain_routers_for_each(igp_domain_t * domain,
				  FRadixTreeForEach for_each,
				  void * ctx);
  // ----- igp_domain_dump ------------------------------------------
  int igp_domain_dump(gds_stream_t * stream, igp_domain_t * domain);
  // ----- igp_domain_info ------------------------------------------
  void igp_domain_info(gds_stream_t * stream, igp_domain_t * domain);

  // -----[ igp_domain_compute ]-------------------------------------
  /**
   * Compute the routing tables of all the routers within an IGP
   * domain.
   *
   * \param domain   is the target IGP domain.
   * \param keep_spt is a flag that, if true, forces the IGP model to
   *                 keep the computed shortest-path trees (SPTs).
   * \retval 0 on success, -1 on error.
   */
  int igp_domain_compute(igp_domain_t * domain, int keep_spt);

  
  ///////////////////////////////////////////////////////////////////
  // LIST OF IGP DOMAINS
  ///////////////////////////////////////////////////////////////////

  // -----[ igp_domains_create ]-------------------------------------
  igp_domains_t * igp_domains_create();
  // -----[ igp_domains_destroy ]------------------------------------
  void igp_domains_destroy(igp_domains_t ** domains_ref);
  // -----[ igp_domains_find ]---------------------------------------
  igp_domain_t * igp_domains_find(igp_domains_t * domains, uint16_t id);
  // -----[ igp_domains_add ]----------------------------------------
  int igp_domains_add(igp_domains_t * domains, igp_domain_t * domain);
  // -----[ igp_domains_for_each ]-----------------------------------
  int igp_domains_for_each(igp_domains_t * domains,
			   FIGPDomainsForEach for_each, void * ctx);


#ifdef __cplusplus
}
#endif

#endif
