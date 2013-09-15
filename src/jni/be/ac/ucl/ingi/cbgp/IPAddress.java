// ==================================================================
// @(#)IPAddress.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// $Id: IPAddress.java,v 1.5 2009-08-31 09:40:35 bqu Exp $
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

// -----[ IPAddress ]------------------------------------------------
/**
 * 
 */
public class IPAddress
{

    // -----[ private attributes of the address ]--------------------
    private byte [] abAddress= null;

    // -----[ IPAddress ]--------------------------------------------
    /**
     * IPAddress's constructor.
     */
    public IPAddress(byte bA, byte bB, byte bC, byte bD) {
    	abAddress= new byte[4];
    	abAddress[0]= bA;
    	abAddress[1]= bB;
    	abAddress[2]= bC;
    	abAddress[3]= bD;
    }

    // -----[ byte2int ]---------------------------------------------
    /**
     * Convert a signed byte (Java) to an unsigned byte stored in an
     * integer. We need this to correctly handle bytes coming from the
     * C part of C-BGP and that are unsigned bytes.
     */
    protected static int byte2int(byte b) {
    	return (b >= 0)?b:(256+b);
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Converts this IP address to a String.
     */
    public String toString() {
    	return byte2int(abAddress[0])+"."+
	    	byte2int(abAddress[1])+"."+
	    	byte2int(abAddress[2])+"."+
	    	byte2int(abAddress[3]);
    }

    // -----[ equals ]-----------------------------------------------
    public boolean equals(IPAddress addr) {
    	return addr.toString().equals(toString());
    }

}
