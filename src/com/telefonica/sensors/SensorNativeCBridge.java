package com.telefonica.sensors;

import org.json.JSONException;

import com.phonegap.api.Plugin;
import com.telefonica.sgap.SensorGapPlugin;

import android.util.Log;

/**
 *  Class that publish an static method with callbacks from sensors
 * 
 * @author jmcf
 *
 */
public class SensorNativeCBridge {
	
	
	private static SensorGapPlugin plugin; 
	
	/**
	 *  Invoked by the native layer
	 * 
	 * @param handle
	 * @param event
	 */
	public static void sensorDataCB(int handle, String event) throws JSONException {

		Log.d("sensors-java", "Event received!!!!!" + event);
		
		// String.valueOf(handle)
		plugin.sensorEvent(handle,event);
		
	}
	
	/**
	 *  Sets the plugin class to be called
	 * 
	 * 
	 * @param p
	 */
	public static void setPlugin(Plugin p) {
		plugin = (SensorGapPlugin)p;
	}
}
