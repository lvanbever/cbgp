// ==================================================================
// @(#)ospf_deflection.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 8/09/2005
// $Id: ospf_deflection.h,v 1.3 2009-03-24 16:22:02 bqu Exp $
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

#ifndef __NET_OSPF_DEFLECTION__H__
#define __NET_OSPF_DEFLECTION__H__

#ifdef OSPF_SUPPORT


//----- ospf_deflection_test() -------------------------------------------------
int ospf_deflection_test(uint16_t uOspfDomain);
// ----- ospf_domain_deflection ------------------------------------------------
void ospf_domain_deflection(SIGPDomain * pDomain); 




#endif
#endif
