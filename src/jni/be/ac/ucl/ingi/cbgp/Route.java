// ==================================================================
// @(#)Route.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: Route.java,v 1.3 2009-08-31 09:40:35 bqu Exp $
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

package be.ac.ucl.ingi.cbgp; 


// -----[ Route ]----------------------------------------------------
/**
 * This is a basic implementation of the RouteInterface.
 */
public class Route {

    // -----[ protected attributes of the route ]-------------------
    protected final IPPrefix prefix;
    protected final RouteEntry [] entries;
    
    // -----[ Route ]-----------------------------------------------
    public Route(IPPrefix prefix, RouteEntry [] entries) {
    	this.prefix= prefix;
    	this.entries= entries;
    }

    // -----[ getOutIface ]-----------------------------------------
    public IPAddress getOutIface() {
    	return entries[0].oif;
    }
    
    // -----[ getGateway ]------------------------------------------
    public IPAddress getGateway() {
    	return entries[0].gateway;
    }

    // -----[ getPrefix ]-------------------------------------------
    public IPPrefix getPrefix() {
    	return prefix;
    }
    
    // -----[ getEntries ]------------------------------------------
    public RouteEntry [] getEntries() {
    	return entries;
    }
    
    // -----[ getEntriesCount ]-------------------------------------
    public int getEntriesCount() {
    	return entries.length;
    }
    
}
