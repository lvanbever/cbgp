// ==================================================================
// @(#)net_ospf.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 15/07/2003
// $Id: net_ospf.c,v 1.7 2009-03-24 15:58:43 bqu Exp $
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

#include <libgds/cli_ctx.h>
#include <cli/common.h>
#include <cli/net_ospf.h>
#include <net/ospf.h>
#include <net/igp_domain.h>
#include <net/net_types.h>
#include <ui/rl.h>
#include <libgds/stream.h>


#ifdef OSPF_SUPPORT

// ----- cli_net_node_ospf_area ------------------------------------
/**
 * context: {node}
 * tokens: {addr, area}
 */
int cli_net_node_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  net_node_t * pNode;
  ospf_area_t uArea;

  // Get node from context
  pNode= (net_node_t *) cli_context_get_item_at_top(pContext);

  // Add ospf area to node
  tokens_get_uint_at(pTokens, 1, &uArea);
  if (node_add_OSPFArea(pNode, uArea) < 0) {
    cli_set_user_error(cli_get(), "unexpected error");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_node_ospf_domain ------------------------------------
/**
 * context: {node}
 * tokens: {addr, igp-domain-id}
 */
int cli_net_node_ospf_domain(SCliContext * pContext, STokens * pTokens)
{
  net_node_t * pNode;
  uint uIGPDomain;

  // Get node from context
  pNode= (net_node_t *) cli_context_get_item_at_top(pContext);
  // Add ospf domain id to node
  tokens_get_uint_at(pTokens, 1, &uIGPDomain);
  SIGPDomain * pIGPDomain = get_igp_domain((uint16_t)uIGPDomain); //creating and registering domain
  igp_domain_add_router(pIGPDomain, pNode);

  return CLI_SUCCESS;
}

// ----- cli_net_subnet_ospf_area ------------------------------------
/**
 * context: {subnet}
 * tokens: {area}
 */
int cli_net_subnet_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  net_subnet_t * pSubnet;
  ospf_area_t uArea;

  // Get node from context
  pSubnet= (net_subnet_t *) cli_context_get_item_at_top(pContext);

  // set subnet ospf area
  tokens_get_uint_at(pTokens, 1, &uArea);
  if (subnet_set_OSPFArea(pSubnet, uArea) != 0) {
    LOG_SEVERE("Warning: correctness of ospf are info of links on subnet is not guaranteed!\n");
    return CLI_SUCCESS;
  }

  return CLI_SUCCESS;
}

// ----- cli_register_net_subnet_ospf --------------------------------------
/**
 *
 */
int cli_register_net_subnet_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_subnet_ospf_area,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL,
					   pSubCmds, NULL));
}

// ----- cli_register_net_node_ospf --------------------------------------
/**
 *
 */
int cli_register_net_node_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds= cli_cmds_create();
  SCliParams * pParams;

  pParams= cli_params_create();
  cli_params_add(pParams, "<igp-domain-id>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("domain",
					cli_net_node_ospf_domain,
					NULL, pParams));
  
  pParams= cli_params_create();
  cli_params_add(pParams, "<area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_node_ospf_area,
					NULL, pParams));
  return cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL,
					   pSubCmds, NULL));
}

// ----- cli_net_node_link_ospf_area ------------------------------------
/**
 * context: {node, link} 
 * tokens: {node-address, destination-prefix, area}
 */
int cli_net_node_link_ospf_area(SCliContext * pContext, STokens * pTokens)
{
  net_node_t * pNode;
  ospf_area_t tArea;
  int iReturn = CLI_SUCCESS;
  net_iface_t * pLink;
  
  pLink = (net_iface_t *) cli_context_get_item_at_top(pContext);
  pNode = (net_node_t *) cli_context_get_item_at(pContext, 0);
  
  if (tokens_get_uint_at(pTokens, 2, &tArea)) {
    LOG_SEVERE("Error: invalid value for ospf-area (%s)\n",
	       tokens_get_string_at(pTokens, 2));
    return CLI_ERROR_COMMAND_FAILED;
  }
  //link_get_prefix(pLink, &sDestPrefix);
  
  switch(link_set_ospf_area(pLink, tArea)){
    case  OSPF_LINK_OK : 
    break;
    
    case  OSPF_LINK_TO_MYSELF_NOT_IN_AREA : 
            LOG_SEVERE("Warning: ospf area link is changed\n");
    break;
    
    case  OSPF_SOURCE_NODE_NOT_IN_AREA    : 
            LOG_SEVERE("Error: source node must belong to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_DEST_NODE_NOT_IN_AREA      : 
            LOG_SEVERE("Error: destination node must belongs to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_DEST_SUBNET_NOT_IN_AREA    : 
            LOG_SEVERE("Error: destination subnet must belongs to area\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    
    case  OSPF_SOURCE_NODE_LINK_MISSING   : //never verified because link is found in context
            LOG_SEVERE("Error: link is not present\n");
	    iReturn = CLI_ERROR_COMMAND_FAILED;
    break;
    default : LOG_SEVERE("Error: unknown reasons\n");
	      iReturn = CLI_ERROR_COMMAND_FAILED;
  }
  
  return iReturn;
}

// ----- cli_register_net_node_link_ospf --------------------------------
void cli_register_net_node_link_ospf(SCliCmds * pCmds)
{
  SCliCmds * pSubCmds = cli_cmds_create();
  SCliParams * pParams = cli_params_create();

  cli_params_add(pParams, "<ospf-area>", NULL);
  cli_cmds_add(pSubCmds, cli_cmd_create("area",
					cli_net_node_link_ospf_area,
					NULL, pParams));
  
  cli_cmds_add(pCmds, cli_cmd_create("ospf", NULL, pSubCmds, NULL));
}
#endif


