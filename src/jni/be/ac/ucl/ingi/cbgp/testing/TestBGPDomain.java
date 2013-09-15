// ==================================================================
// @(#)TestBGPDomain.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestBGPDomain.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import java.util.Vector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;

public class TestBGPDomain {
	
	CBGP cbgp;
	
	@Before
	public void setUp() {
		cbgp= new CBGP();
		try {
			cbgp.init(null);
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@After
	public void tearDown() {
		try {
			cbgp.destroy();
			cbgp= null;
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test
	public void testGetDomains() {
		try {
			IGPDomain domain1= cbgp.netAddDomain(123);
			domain1.addNode("1.0.0.1");
			IGPDomain domain2= cbgp.netAddDomain(456);
			domain2.addNode("1.0.0.2");
			cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
			cbgp.bgpAddRouter("RT1", "1.0.0.2", 456);
			Vector domains= cbgp.bgpGetDomains();
			assertNotNull(domains);
			assertEquals(2, domains.size());
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
}
