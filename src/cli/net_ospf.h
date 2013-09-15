// ==================================================================
// @(#)net_ospf.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 15/07/2003
// $Id: net_ospf.h,v 1.4 2009-03-24 15:58:43 bqu Exp $
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

#ifdef OSPF_SUPPORT

#ifndef __CLI_NET_OSPF_H__
#define __CLI_NET_OSPF_H__


#include <libgds/cli.h>

// ----- cli_net_node_by_addr ---------------------------------------
// extern SNetNode * cli_net_node_by_addr(char * pcAddr);
// ----- cli_register_net -------------------------------------------
// extern int cli_register_net(SCli * pCli);
// ----- cli_register_net_node_ospf --------------------------------------
extern int cli_register_net_node_ospf(SCliCmds * pCmds);
// ----- cli_register_net_ospf --------------------------------------
//extern int cli_register_net_ospf(SCliCmds * pCmds);
// ----- cli_register_net_subnet_ospf --------------------------------------
extern int cli_register_net_subnet_ospf(SCliCmds * pCmds);
// ----- cli_net_node_link_ospf_area ------------------------------------
extern int cli_net_node_link_ospf_area(SCliContext * pContext, STokens * pTokens);
// ----- cli_register_net_node_link_ospf --------------------------------
extern void cli_register_net_node_link_ospf(SCliCmds * pCmds);
#endif
#endif
