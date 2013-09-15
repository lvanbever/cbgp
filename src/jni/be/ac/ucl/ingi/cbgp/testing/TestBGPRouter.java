// ==================================================================
// @(#)TestBGPRouter.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestBGPRouter.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;

public class TestBGPRouter {
	
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
	public void testNormal() {
		try {
			IGPDomain domain= cbgp.netAddDomain(123);
			domain.addNode("1.0.0.1");
			Router router1= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
			assertNotNull(router1);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
	@Test(expected=CBGPException.class)
	public void testNoNode() throws CBGPException {
		cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
	}
	
	@Test(expected=CBGPException.class)
	public void testDuplicate() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		cbgp.bgpAddRouter("RT2", "1.0.0.1", 123);
	}
	
	@Test
	public void testAddNetwork() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		Router router= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		router.addNetwork("10.0.1/24");
		router.addNetwork("10.0.2/24");
	}
	
	@Test(expected=CBGPException.class)
	public void testAddNetworkDup() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		domain.addNode("1.0.0.1");
		Router router= cbgp.bgpAddRouter("RT1", "1.0.0.1", 123);
		router.addNetwork("10.0.1/24");
		router.addNetwork("10.0.1/24");
	}
	
}
