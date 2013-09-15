// ==================================================================
// @(#)rexford.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/07/2003
// $Id: rexford.c,v 1.2 2009-06-25 14:31:03 bqu Exp $
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

#include <assert.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/tokenizer.h>

#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <net/util.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/rexford.h>
#include <bgp/peer.h>

// -----[ _rexford_relation_to_peer_type ]-----------------------------
static inline int _rexford_relation_to_peer_type(int iRelation,
						 peer_type_t * ptPeerType)
{
  switch (iRelation) {
  case REXFORD_REL_PROV_CUST:
    *ptPeerType= ASLEVEL_PEER_TYPE_CUSTOMER;
    return 0;
  case REXFORD_REL_PEER_PEER:
    *ptPeerType= ASLEVEL_PEER_TYPE_PEER;
    return 0;
  }
  return -1;
}

// -----[ rexford_parser ]-------------------------------------------
/**
 * Load an AS-level topology in the Subramanian et al format. The
 * file format is as follows. Each line describes a directed
 * relationship between a two ASes:
 *   <AS1-number> <AS2-number> <peering-type>
 * where peering type can be
 *   0 for a peer-to-peer relationship
 *   1 for a provider (AS1) to customer (AS2) relationship
 * A relationship among two ASes is described in one direction only.
 */
int rexford_parser(FILE * file, as_level_topo_t * topo,
		   int * line_number)
{
  char line[80];
  asn_t asn1, asn2;
  unsigned int relation;
  int error= 0;
  net_link_delay_t delay;
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;
  as_level_domain_t * domain1, * domain2;
  peer_type_t peer_type;

  *line_number= 0;

  // Parse input file
  tokenizer= tokenizer_create(" \t", NULL, NULL);
  
  while ((!feof(file)) && (!error)) {
    if (fgets(line, sizeof(line), file) == NULL)
      break;
    (*line_number)++;
    
    // Skip comments starting with '#'
    if (line[0] == '#')
      continue;
    
    if (tokenizer_run(tokenizer, line) != TOKENIZER_SUCCESS) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    tokens= tokenizer_get_tokens(tokenizer);
    
    // Set default value for optional parameters
    delay= 0;
    
    // Get and check mandatory parameters
    if (tokens_get_num(tokens) < 3) {
      error= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }

    // Get and check ASNs
    if (str2asn(tokens_get_string_at(tokens, 0), &asn1) ||
	str2asn(tokens_get_string_at(tokens, 1), &asn2)) {
      error= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    
    // Get and check business relationship
    if ((tokens_get_uint_at(tokens, 2, &relation) != 0) ||
	(_rexford_relation_to_peer_type(relation, &peer_type) != 0)) {
      error= ASLEVEL_ERROR_INVALID_RELATION;
      break;
    }
    
    // Get optional parameters
    if (tokens_get_num(tokens) > 3) {
      if (str2delay(tokens_get_string_at(tokens, 3), &delay)) {
	error= ASLEVEL_ERROR_INVALID_DELAY;
	break;
      }
    }
    
    // Limit number of parameters
    if (tokens_get_num(tokens) > 4) {
      STREAM_ERR(STREAM_LEVEL_SEVERE,
	      "Error: too many arguments in topology, line %u\n",
	      *line_number);
      error= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }
    
    // Add/get domain 1
    if (!(domain1= aslevel_topo_get_as(topo, asn1)) &&
	!(domain1= aslevel_topo_add_as(topo, asn1))) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add/get domain 2
    if (!(domain2= aslevel_topo_get_as(topo, asn2)) &&
	!(domain2= aslevel_topo_add_as(topo, asn2))) {
      error= ASLEVEL_ERROR_UNEXPECTED;
      break;
    }
    
    // Add link in both directions
    error= aslevel_as_add_link(domain1, domain2, peer_type, NULL);
    if (error != ASLEVEL_SUCCESS)
      break;
    peer_type= aslevel_reverse_relation(peer_type);
    error= aslevel_as_add_link(domain2, domain1, peer_type, NULL);
    if (error != ASLEVEL_SUCCESS)
      break;
    
  }

  tokenizer_destroy(&tokenizer);
  return error;
}
