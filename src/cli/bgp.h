// ==================================================================
// @(#)bgp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: bgp.h,v 1.5 2009-03-24 15:57:51 bqu Exp $
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

#ifndef __CLI_BGP_H__
#define __CLI_BGP_H__

#include <libgds/cli.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- cli_register_bgp -----------------------------------------
  void cli_register_bgp(cli_cmd_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* __CLI_BGP_H__ */
