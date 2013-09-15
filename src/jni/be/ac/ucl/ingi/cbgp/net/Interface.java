// ==================================================================
// @(#)Link.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/02/2005
// $Id: Interface.java,v 1.2 2009-08-31 09:44:52 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.net; 

import be.ac.ucl.ingi.cbgp.*;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ Link ]-----------------------------------------------------
/**
 * This class is a container for a link.
 */
public class Interface extends Element {
	
	public static final long MAX_METRIC= 4294967295l;
	
	public static final String TYPE_LO     = "lo";
	public static final String TYPE_RTR    = "rtr";
	public static final String TYPE_PTP    = "ptp";
	public static final String TYPE_PTMP   = "ptmp";
	public static final String TYPE_VIRTUAL= "tun";

    // -----[ protected attributes ]---------------------------------
    protected IPPrefix id; // Interface address / mask
    protected long lDelay;

    // -----[ Proxy management native methods ]----------------------
    private native void _proxy_finalize();

    // -----[ Link ]-------------------------------------------------
    /**
     * Link's constructor.
     */
    protected Interface(CBGP cbgp, IPPrefix id, long lDelay) {
    	super(cbgp);
    	this.id= id;
    	this.lDelay= lDelay;
    }

    // -----[ finalize ]---------------------------------------------
    protected void finalize() {
    	_proxy_finalize();
    }

    // -----[ getType ]----------------------------------------------
    public native synchronized String getType();
    
    // -----[ getCapacity ]------------------------------------------
    public native synchronized long getCapacity()
    	throws CBGPException;
    
    // -----[ setCapacity ]------------------------------------------
    public native synchronized void setCapacity(long capacity)
    	throws CBGPException;
    
    // -----[ getDelay ]---------------------------------------------
    /**
     * Return the link's propagation delay.
     */
    public native synchronized long getDelay();
    
    // -----[ setDelay ]---------------------------------------------
    /**
     * Set the link's propagation delay.
     */
    public native synchronized void setDelay(long delay); 
    
    // -----[ getLoad ]----------------------------------------------
    /**
     * Return the link's traffic load.
     */
    public native synchronized long getLoad()
    	throws CBGPException;
    
    // -----[ getPrefix ]-------------------------------------------
    public IPPrefix getPrefix() {
    	return id;
    }
    
    // -----[ getId ]-----------------------------------------------
    /**
     * Return the interface's ID (address / mask).
     */
    public String getId() {
    	return getHead().getId()+" "+id.toString();
    }
    
    // -----[ getWeight ]--------------------------------------------
    /**
     * Return the link's IGP weight.
     */
    public native synchronized long getWeight()
    	throws CBGPException;
    
    // -----[ setWeight ]-------------------------------------------
    /**
     * Set the link's IGP weight.
     */
    public native synchronized void setWeight(long lWeight)
		throws CBGPException;

    // -----[ getState ]---------------------------------------------
    /**
     *
     */
    public native synchronized boolean getState()
    	throws CBGPException;

    // -----[ setState ]---------------------------------------------
    /**
     *
     */
    public native synchronized void setState(boolean state)
		throws CBGPException;
    
    // -----[ getHead ]---------------------------------------------
    /**
     * Returns the node where this interface is defined.
     */
    public native synchronized Node getHead();
    
    // -----[ getTail ]----------------------------------------------
    /**
     * Returns the tail-end of the link departing from this
     * interface. This only works for links of type RTR, PTP and
     * PTMP. On RTR and PTP links, the returned object is a Node. On
     * PTMP links, the returned object is a Subnet. 
     */
    public native synchronized Element getTail();
    
    // -----[ getTailInterface ]-------------------------------------
    /**
     * Returns the other interface on the connected link. This only
     * works for links of type RTR and PTP. On other link types, this
     * method will return null.
     */
    public native synchronized Interface getTailInterface();

    // -----[ toString ]---------------------------------------------
    /**
     * Convert this link to a String.
     */
    public String toString() {
    	String s= "";

    	/* Attributes */
    	s+= id.address+"/"+id.bMask;

    	return s;
    }

}
