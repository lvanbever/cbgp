// ==================================================================
// @(#)AllTests.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: AllTests.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

import junit.framework.JUnit4TestAdapter;
import junit.framework.Test;

import org.junit.runner.JUnitCore;
import org.junit.runner.Result;
import org.junit.runner.RunWith;
import org.junit.runners.Suite;
import org.junit.runners.Suite.SuiteClasses;

@RunWith(Suite.class)
@SuiteClasses({
	TestSession.class,
	TestScript.class,
	TestNode.class,
	TestSubnet.class,
	TestLink.class,
	TestIGPDomain.class,
	TestBGPDomain.class,
	TestBGPRouter.class,
	TestBGPRoutes.class,
	TestMisc.class,
	TestRecordRoute.class,
	TestTraceRoute.class
})
public class AllTests {
	
	public static void main(String[] args) {
		System.out.println("Running JUnit tests (version:"+(new JUnitCore()).getVersion()+")");
		/*TestResult result = new TestResult();
		suite().run(result);*/
		Result result= JUnitCore.runClasses(new Class [] {AllTests.class});
		System.out.println(result.toString());
	}
	
	public static Test suite() {
		return new JUnit4TestAdapter(AllTests.class);
	}
		
}
