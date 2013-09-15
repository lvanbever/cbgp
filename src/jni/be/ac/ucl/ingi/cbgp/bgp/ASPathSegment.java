// ==================================================================
// @(#)ASPathSegment.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2004
// $Id: ASPathSegment.java,v 1.3 2009-08-31 09:42:43 bqu Exp $
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

import java.lang.Exception;
import java.util.Vector;

// -----[ ASPathSegment ]--------------------------------------------
/**
 * This class represents an AS-Path segment.
 */
public class ASPathSegment {

    // -----[ constants ]--------------------------------------------
    public static final int AS_PATH_SEGMENT_SET     = 1;
    public static final int AS_PATH_SEGMENT_SEQUENCE= 2;

    // -----[ protected attributes ]---------------------------------
    protected int iType;
    protected Vector<Integer> ases= null;

    // -----[ ASPathSegment ]----------------------------------------
    /**
     * ASPathSegment's constructor.
     */
    public ASPathSegment(int iType)
    	throws Exception {
    	this.ases= new Vector<Integer>();
    	if ((iType != AS_PATH_SEGMENT_SET) &&
    			(iType != AS_PATH_SEGMENT_SEQUENCE))
    		throw new Exception("Invalid AS-Path segment type "+iType);
    	this.iType= iType;
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Append an AS-number to the AS-Path segment.
     */
    public void append(int iAS)
    	throws Exception {
    	if ((iAS < 0) || (iAS > 65535))
    		throw new Exception("Invalid AS number "+iAS);
		ases.add(new Integer(iAS));
    }

    // -----[ get ]--------------------------------------------------
    /**
     * Return the AS-Path segment element at the given position.
     */
    public int get(int iIndex) throws Exception {
    	if ((iIndex < 0) || (iIndex >= ases.size()))
    		throw new Exception("Invalid AS-Path segment index "+iIndex);
    	return ((Integer) ases.get(iIndex)).intValue();
    }
    
    // -----[ count ]------------------------------------------------
    /**
     * Return the AS-path segment length.
     */
    public int count() {
    	return ases.size();
    }

    // -----[ getType ]----------------------------------------------
    /**
     * Return the AS-Path segment's type.
     */
    public int getType() {
    	return iType;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the AS-Path segment to a String.
     */
    public String toString() {
    	String s="";

    	if (iType == AS_PATH_SEGMENT_SET)
    		s+= "{ ";
    	
    	for (int iIndex= ases.size(); iIndex > 0; iIndex--) {
    		if (iIndex < ases.size())
    			s+= " ";
    		s+= ases.get(iIndex-1);
    	}
	
    	if (iType == AS_PATH_SEGMENT_SET)
    		s+= " }";

    	return s;
    }

}
