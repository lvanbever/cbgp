// ==================================================================
// @(#)ProxyObject.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/02/2006
// $Id: ProxyObject.java,v 1.7 2009-08-31 09:40:35 bqu Exp $
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

// -----[ ProxyObject ]----------------------------------------------
public class ProxyObject extends Object {

    // -----[ objectId ]---------------------------------------------
    /**
     * This is a replacement for Object's hashCode() method that ensures
     * the returned hashcode is unique.
     */ 
    @SuppressWarnings("unused")
	private long objectId;
	
    // -----[ ProxyObject ]------------------------------------------
    protected ProxyObject(CBGP cbgp) {
    }
	
    // -----[ getCBGP ]----------------------------------------------
    public native CBGP getCBGP();
	
    // -----[ finalize ]---------------------------------------------
    protected void finalize() {
    	_jni_unregister();
    }
	
    // -----[ _jni_unregister ]--------------------------------------
    private native void _jni_unregister();

}
