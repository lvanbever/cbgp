// ==================================================================
// @(#)LinkMetrics.java
//
// @author Bruno Quoitin (bqu@info.ucl.ac.be)
// @date 30/05/2007
// @lastdate 30/05/2007
// ==================================================================

package be.ac.ucl.ingi.cbgp; 

import java.util.HashMap;

import be.ac.ucl.ingi.cbgp.exceptions.UnknownMetricException;

// -----[ LinkMetrics ]-----------------------------------------------
/**
 * This class represents the metrics associated with one link.
 */
public class LinkMetrics
{

    // -----[ supported metrics (constants) ]------------------------
    public static final String CAPACITY= "capacity";
    public static final String DELAY   = "delay";
    public static final String WEIGHT  = "weight";

    // -----[ internal fields ]--------------------------------------
    protected HashMap<String, Object> metrics;

    // -----[ LinkMetrics ]------------------------------------------
    public LinkMetrics(int iDelay, int iWeight, int iCapacity)
    {
    	metrics= new HashMap<String, Object>();
    	metrics.put(CAPACITY, new Integer(iCapacity));
    	metrics.put(DELAY, new Integer(iDelay));
    	metrics.put(WEIGHT, new Integer(iWeight));
    }

    // -----[ LinkMetrics ]------------------------------------------
    public LinkMetrics()
    {
	metrics= null;
    }

    // -----[ setMetric ]--------------------------------------------
    public void setMetric(String metricID, Object metricValue)
    {
	if (metrics == null)
	    metrics= new HashMap<String, Object>();
	metrics.put(metricID, metricValue);
    }

    // -----[ getMetric ]--------------------------------------------
    public Object getMetric(String metricID) throws UnknownMetricException
    {
	Object metricObj;
	if (metrics == null)
	    throw new UnknownMetricException();
	metricObj= metrics.get(metricID);
	if (metricObj == null)
	    throw new UnknownMetricException();
	return metricObj;
    }

    // -----[ hasMetric ]--------------------------------------------
    public boolean hasMetric(String metricID)
    {
	if ((metrics == null) || (metrics.get(metricID) == null))
	    return false;
	return true;
    }

    // -----[ getCapacity ]------------------------------------------
    public int getCapacity() throws UnknownMetricException
    {
	return ((Integer) getMetric(CAPACITY)).intValue();
    }

    // -----[ getDelay ]---------------------------------------------
    public int getDelay() throws UnknownMetricException
    {
	return ((Integer) getMetric(DELAY)).intValue();
    }

    // -----[ getWeight ]--------------------------------------------
    public int getWeight() throws UnknownMetricException
    {
	return ((Integer) getMetric(WEIGHT)).intValue();
    }

}
