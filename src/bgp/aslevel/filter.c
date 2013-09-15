// ==================================================================
// @(#)filter.c
//
// Provide filtering capabilities to prune AS-level topologies from
// specific classes of nodes / edges. Example of such filters include
// - remove all stubs
// - remove all single-homed stubs
// - remove all peer-to-peer links
// - remove all but top-level domains
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: filter.c,v 1.2 2009-08-31 09:35:34 bqu Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/filter.h>
#include <bgp/aslevel/types.h>
#include <bgp/aslevel/util.h>

// -----[ aslevel_filter_str2filter ]--------------------------------
/**
 * Convert a filter name to its identifier.
 */
int aslevel_filter_str2filter(const char * str,
			      aslevel_filter_t * filter)
{
  if (!strcmp(str, "stubs")) {
    *filter= ASLEVEL_FILTER_STUBS;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(str, "single-homed-stubs")) {
    *filter= ASLEVEL_FILTER_SHSTUBS;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(str, "peer-to-peer")) {
    *filter= ASLEVEL_FILTER_P2P;
    return ASLEVEL_SUCCESS;
  } else if (!strcmp(str, "keep-top")) {
    *filter= ASLEVEL_FILTER_KEEP_TOP;
    return ASLEVEL_SUCCESS;
  }
  return ASLEVEL_ERROR_UNKNOWN_FILTER;
}

// -----[ aslevel_filter_topo ]--------------------------------------
/**
 * Filter the given topology with 
 */
int aslevel_filter_topo(as_level_topo_t * topo, aslevel_filter_t filter)
{
  unsigned int index, index2;
  as_level_domain_t * domain;
  as_level_link_t * link;
  unsigned int num_customers, num_providers;
  AS_SET_DEFINE(auSet);
  unsigned int num_nodes_removed= 0, num_edges_removed= 0;

  AS_SET_INIT(auSet);

  // Identify domains to be removed
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    num_customers= aslevel_as_num_customers(domain);

    switch (filter) {

    case ASLEVEL_FILTER_STUBS:
      // Remove the domain if it has no customers and at least one
      // provider (to avoid removing top domains)
      if (num_customers == 0) {
	/*uNumProviders= aslevel_as_num_providers(pDomain);
	  if (uNumProviders > 0)*/
	  AS_SET_PUT(auSet, domain->asn);
      }
      break;

    case ASLEVEL_FILTER_SHSTUBS:
      // Remove the domain if it has no customers and exactly one single
      // provider
      if (num_customers == 0) {
	num_providers= aslevel_as_num_providers(domain);
	if (num_providers == 1)
	  AS_SET_PUT(auSet, domain->asn);
      }
      break;

    case ASLEVEL_FILTER_P2P:
      // Remove peer-to-peer links 
      num_providers= aslevel_as_num_providers(domain);
      if (num_providers == 0)
	break;
      index2= 0;
      while (index2 < ptr_array_length(domain->neighbors)) {
	link= (as_level_link_t *) domain->neighbors->data[index2];
	num_providers= aslevel_as_num_providers(link->neighbor);
	if (num_providers == 0)
	  break;
	if (link->peer_type == ASLEVEL_PEER_TYPE_PEER) {
	  ptr_array_remove_at(domain->neighbors, index2);
	  continue;
	}
	index2++;
      }
      if (ptr_array_length(domain->neighbors) == 0)
	AS_SET_PUT(auSet, domain->asn);
      break;

    case ASLEVEL_FILTER_KEEP_TOP:
      // Remove the domain if it has providers
      num_providers= aslevel_as_num_providers(domain);
      if (num_providers > 0)
	AS_SET_PUT(auSet, domain->asn);
      break;

    default:
      return ASLEVEL_ERROR_UNKNOWN_FILTER;

    }
  }

  // Remove links to/from identified domains
  for (index= 0; index < aslevel_topo_num_nodes(topo); index++) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    index2= 0;
    while (index2 < ptr_array_length(domain->neighbors)) {
      link= (as_level_link_t *) domain->neighbors->data[index2];
      if (IS_IN_AS_SET(auSet, link->neighbor->asn)) {
	ptr_array_remove_at(domain->neighbors, index2);
	num_edges_removed++;
	continue;
      }
      index2++;
    }
  }

  // Remove identified domains
  index= 0;
  while (index < aslevel_topo_num_nodes(topo)) {
    domain= (as_level_domain_t *) topo->domains->data[index];
    if (IS_IN_AS_SET(auSet, domain->asn)) {
      ptr_array_remove_at(topo->domains, index);
      num_nodes_removed++;
      continue;
    }
    index++;
  }

  //stream_printf(gdserr, "\tnumber of nodes removed: %d\n", num_nodes_removed);
  //stream_printf(gdserr, "\tnumber of edges removed: %d\n", num_edges_removed);

  return ASLEVEL_SUCCESS;
}
