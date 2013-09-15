// ==================================================================
// @(#)TestBGPRoutes.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestBGPRoutes.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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
import be.ac.ucl.ingi.cbgp.bgp.Peer;
import be.ac.ucl.ingi.cbgp.bgp.Route;
import be.ac.ucl.ingi.cbgp.bgp.Router;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestBGPRoutes {
	
	CBGP cbgp;
	IGPDomain domain;
	Node node1, node2;
	Router router1, router2;
	
	@Before
	public void setUp() throws CBGPException {
		cbgp= new CBGP();
		cbgp.init(null);
		domain= cbgp.netAddDomain(1);
		node1= domain.addNode("1.0.0.1");
		node2= domain.addNode("1.0.0.2");
		router1= cbgp.bgpAddRouter("R1", "1.0.0.1", 1);
		node1.addLTLLink(node2, false).setWeight(1);
		node2.addLTLLink(node1, false).setWeight(2);
		domain.compute();
		router2= cbgp.bgpAddRouter("R2", "1.0.0.2", 1);
		Peer peer12= router1.addPeer("1.0.0.2", 1);
		Peer peer21= router2.addPeer("1.0.0.1", 1);
		peer12.openSession();
		peer21.openSession();
		cbgp.simRun();
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
	public void testGetRIB_All() throws CBGPException {
		router1.addNetwork("255/8");
		Vector<Route> routes= router1.getRIB("*");
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test
	public void testGetRIB_Exact() throws CBGPException {
		router1.addNetwork("255/8");
		Vector<Route> routes= router1.getRIB("255/8");
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test
	public void testGetRIB_Best() throws CBGPException {
		router1.addNetwork("255/8");
		Vector<Route> routes= router1.getRIB("255.0.0.1");
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test(expected=CBGPException.class)
	public void testGetRIB_BadDest1() throws CBGPException {
		router1.getRIB("");
	}
	
	@Test(expected=CBGPException.class)
	public void testGetRIB_BadDest2() throws CBGPException {
		router1.getRIB("1.2.3.4.5");
	}
	
	@Test(expected=CBGPException.class)
	public void testGetRIB_BadDest3() throws CBGPException {
		router1.getRIB("/29");
	}
	
	@Test
	public void testGetAdjRIB_All() throws CBGPException {
		router1.addNetwork("255/8");
		cbgp.simRun();
		Vector<Route> routes= router2.getAdjRIB(null, "*", true);
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test
	public void testGetAdjRIB_Exact() throws CBGPException {
		router1.addNetwork("255/8");
		cbgp.simRun();
		Vector<Route> routes= router2.getAdjRIB(null, "255/8", true);
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test
	public void testGetAdjRIB_Best() throws CBGPException {
		router1.addNetwork("255/8");
		cbgp.simRun();
		Vector<Route> routes= router2.getAdjRIB(null, "255.0.0.1", true);
		assertNotNull(routes);
		assertEquals(1, routes.size());
		assertEquals(routes.get(0).getPrefix().toString(), "255.0.0.0/8");
	}
	
	@Test(expected=CBGPException.class)
	public void testGetAdjRIB_BadPeer() throws CBGPException {
		router2.getAdjRIB("1.0.0.2", "*", true);
	}
	
	@Test(expected=CBGPException.class)
	public void testGetAdjRIB_BadDest1() throws CBGPException {
		router2.getAdjRIB("1.0.0.1", "", true);
	}

	@Test(expected=CBGPException.class)
	public void testGetAdjRIB_BadDest2() throws CBGPException {
		router2.getAdjRIB("1.0.0.1", "1.2.3.4.5", true);
	}

	@Test(expected=CBGPException.class)
	public void testGetAdjRIB_BadDest3() throws CBGPException {
		router2.getAdjRIB("1.0.0.1", "/29", true);
	}
}
