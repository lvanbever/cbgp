// ==================================================================
// @(#)import_meulle.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/2007
// $Id: meulle.c,v 1.2 2009-06-25 14:31:03 bqu Exp $
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
#endif /* HAVE_CONFIG_H */

#include <assert.h>
#include <string.h>

#include <libgds/stream.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/meulle.h>
#include <bgp/aslevel/rexford.h>
#include <bgp/peer.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <net/util.h>

// -----[ meulle_parser ]-------------------------------------------
/**
 * Load an AS-level topology in the "Meulle" format. The
 * file format is as follows. Each line describes a directed
 * relationship between a two ASes:
 *   AS<ASi-number> \t AS<ASj-number> \t <peering-type>
 * where peering-type can be
 *   PEER for a peer-to-peer relationship
 *   P2C  for a provider (ASi) to customer (ASj) relationship
 *   C2P  for a customer (ASi) to provider (ASj) relationship
 * A relationship among two ASes is described in one direction only.
 */
int meulle_parser(FILE * file, as_level_topo_t * topo,
		  int * line_number)
{
  char line[80];
  const char * asn1_str, * asn2_str, * relation;
  asn_t asn1, asn2;
  int error= 0;
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
    
    // Get and check mandatory parameters
    if (tokens_get_num(tokens) != 3) {
      error= ASLEVEL_ERROR_NUM_PARAMS;
      break;
    }

    // Get and check ASNs
    asn1_str= tokens_get_string_at(tokens, 0);
    if (strncmp(asn1_str, "AS", 2) ||
	str2asn(asn1_str+2, &asn1)) {
      error= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }
    asn2_str= tokens_get_string_at(tokens, 1);
    if (strncmp(asn2_str, "AS", 2) ||
	str2asn(asn2_str+2, &asn2)) {
      error= ASLEVEL_ERROR_INVALID_ASNUM;
      break;
    }

    // Get and check business relationship
    relation= tokens_get_string_at(tokens, 2);
    if (!strcmp(relation, "PEER")) {
      peer_type= ASLEVEL_PEER_TYPE_PEER;
    } else if (!strcmp(relation, "P2C")) {
      peer_type= ASLEVEL_PEER_TYPE_CUSTOMER;
    } else if (!strcmp(relation, "C2P")) {
      peer_type= ASLEVEL_PEER_TYPE_PROVIDER;
    } else {
      error= ASLEVEL_ERROR_INVALID_RELATION;
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
    
    // Add link
    error= aslevel_as_add_link(domain1, domain2, peer_type, NULL);
    if (error	!= ASLEVEL_SUCCESS)
      break;
    
  }

  tokenizer_destroy(&tokenizer);
  return error;
}
