// ==================================================================
// @(#)IGPDomain.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 17/03/2006
// $Id: IGPDomain.java,v 1.5 2009-08-31 09:44:52 bqu Exp $
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

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.*;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

// -----[ IGPDomain ]------------------------------------------------
/**
 * This class represents an IGP domain.
 */
public class IGPDomain extends ProxyObject
{

    // -----[ IGP domain types ]-------------------------------------
    public static final int DOMAIN_IGP = 0;
    public static final int DOMAIN_OSPF= 1;

    // -----[ protected attributes ]---------------------------------
    protected int id;
    protected int type;

    // -----[ IGPDomain ]--------------------------------------------
    /**
     * IGPDomain's constructor.
     */
    public IGPDomain(CBGP cbgp, int id, int type) {
    	super(cbgp);
    	this.id= id;
    	this.type= type;
    }

    // -----[ getID ]------------------------------------------------
    /**
     * Returns the domain's ID.
     */
    public int getID() {
    	return id;
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Returns the domain's type.
     */
    public int getType() {
    	return type;
    }
    
    // -----[ addNode ]---------------------------------------------
    public native synchronized Node addNode(String sAddr)
    	throws CBGPException;
    
    // -----[ getNodes ]--------------------------------------------
    public native synchronized Vector<Node> getNodes()
		throws CBGPException;
    
    // -----[ compute ]---------------------------------------------
    public native synchronized void compute()
		throws CBGPException;

    // -----[ typeToString ]-----------------------------------------
    /**
     * Convert an IGP domain type to a String.
     */
    public static String typeToString(int type) {
    	String s;
    	switch (type) {
    	case DOMAIN_IGP:
    		s= "IGP"; break;
    	case DOMAIN_OSPF:
    		s= "OSPF"; break;
    	default:
    		s= "?";
    	}
    	return s;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IGPDomain to a String.
     */
    public String toString() {
    	return "Domain "+id;
    }

}
