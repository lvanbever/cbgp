// ==================================================================
// @(#)BGPDomain.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/03/2006
// $Id: Domain.java,v 1.5 2009-08-31 09:42:43 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.bgp; 

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ BGPDomain ]------------------------------------------------
/**
 * This class represents an AS (BGP domain).
 */
public class Domain extends ProxyObject {

    // -----[ protected attributes ]---------------------------------
    protected int id;

    // -----[ BGPDomain ]--------------------------------------------
    /**
     * BGPDomain's constructor.
     */
    protected Domain(CBGP cbgp, int id) {
    	super(cbgp);
    	this.id= id;
    }

    // -----[ getID ]------------------------------------------------
    /**
     * Returns the domain's ID.
     */
    public int getID() {
    	return id;
    }
    
    // -----[ getRouters ]------------------------------------------
    public native synchronized Vector<Router> getRouters()
		throws CBGPException;
    
    //  -----[ rescan ]-------------------------------------------------
    public native void rescan()
		throws CBGPException;

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this BGPDomain to a String.
     */
    public String toString() {
    	return "AS "+id;
    }

}
