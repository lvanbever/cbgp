// ==================================================================
// @(#)IPRoute.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: IPRoute.java,v 1.4 2009-08-31 09:40:35 bqu Exp $
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

// -----[ IPRoute ]--------------------------------------------------
/**
 * This class is a container for IP route information.
 */
public class IPRoute extends Route {

    // -----[ public constants ]-------------------------------------
	public static final byte NET_ROUTE_DIRECT= 0x01;
    public static final byte NET_ROUTE_STATIC= 0x02;
    public static final byte NET_ROUTE_IGP= 0x04;
    public static final byte NET_ROUTE_BGP= 0x08;

    // -----[ protected attributes of the IP route ]-----------------
    protected byte type;

    // -----[ IPRoute ]----------------------------------------------
    /**
     * IPRoute's constructor.
     */
    public IPRoute(IPPrefix prefix, RouteEntry [] entries, byte type) {
    	super(prefix, entries);

    	/* Attributes */
    	this.type= type;
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Returns the route's type (i.e. the protocol that added the
     * route).
     */
    public byte getType() {
    	return type;
    }

    // -----[ typeToString ]-----------------------------------------
    /**
     * Returns a String version of the given route type.
     */
    public static String typeToString(byte bType) {
    	switch (bType) {
    	case NET_ROUTE_DIRECT: return "DIRECT";
    	case NET_ROUTE_STATIC: return "STATIC";
    	case NET_ROUTE_IGP   : return "IGP";
    	case NET_ROUTE_BGP   : return "BGP";
    	default:
    		return "UNKNOWN";
    	}
    }

    // -----[ toString ]---------------------------------------------
    /**
     * This function converts this route in a String.
     */
    public String toString() {
    	String s= "";

    	/* Flags */
    	s+= "*> ";

    	/* Attributes */
    	s+= prefix;
    	s+= "\t";
    	s+= getOutIface();
    	s+= "\t";
    	s+= typeToString(type);

    	return s;
    }

}
