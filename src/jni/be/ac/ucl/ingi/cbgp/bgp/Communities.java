// ==================================================================
// @(#)Communities.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/03/2006
// $Id: Communities.java,v 1.3 2009-08-31 09:42:43 bqu Exp $
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

// -----[ Communities ]----------------------------------------------
/**
 * This class represents an set of Communities.
 */
public class Communities
{
    
    // -----[ protected attributes ]---------------------------------
    protected Vector<Integer> communities= null;

    // -----[ Communities ]------------------------------------------
    /**
     * Communities's constructor.
     */
    public Communities() {
    	this.communities= new Vector<Integer>();
    }

    // -----[ append ]-----------------------------------------------
    /**
     * Append a new Community.
     */
    public void append(int community) {
    	communities.add(new Integer(community));
    }

    // -----[ getCommunityCount ]-----------------------------------
    public int getCommunityCount() {
    	return communities.size();
    }
    
    // -----[ getCommunity ]----------------------------------------
    /**
     * Return the Community at the given position.
     */
    public int getCommunity(int iIndex) {
    	return ((Integer) communities.get(iIndex)).intValue();
    }
    
    // -----[ communityToString ]------------------------------------
    /**
     * Convert a community to its String representation.
     */
    public static String communityToString(int community) {
    	long lCommunity= (community >= 0)
	    	?community
	    			:(0x100000000L +community);
    	String s= "";
    	s+= (lCommunity >> 16);
    	s+= ":";
    	s+= (lCommunity & 0xffff);
    	return s;
    }

    // -----[ toString ]---------------------------------------------
    /**
     * Convert the Communities to a String.
     */
    public String toString() {
    	String s= "";

    	for (int iIndex= communities.size(); iIndex > 0; iIndex--) {
    		if (iIndex < communities.size()) {
    			s+= " ";
    		}
    		s+= communityToString(getCommunity(iIndex-1));
    	}
    	return s;
    }

}
