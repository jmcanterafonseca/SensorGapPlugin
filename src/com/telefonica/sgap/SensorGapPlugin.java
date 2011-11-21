package com.telefonica.sgap;

import java.util.HashMap;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.util.Log;

import com.phonegap.api.Plugin;
import com.phonegap.api.PluginResult;
import com.telefonica.sensors.SensorNative;
import com.telefonica.sensors.SensorNativeCBridge;

/**
 * Sensor API Phone GAP Plugin
 * 
 * @author jmcf
 * 
 */
public class SensorGapPlugin extends Plugin {
	private SensorNative _native = new SensorNative();
	private Map<String, String> callbacks = new HashMap<String, String>();
	
	/**
	 *  Sets the Pugin attribute of the native callback bridge class
	 * 
	 * 
	 */
	public SensorGapPlugin() {
		SensorNativeCBridge.setPlugin(this);
	}

	/**
	 * 
	 * Executes the actions requested from the Java Layer 
	 * 
	 * Action can be one of [ '_FIND', '_CONNECT','_WATCH','_END_WATCH'] 
	 * 
	 * 
	 */
	public PluginResult execute(String action, JSONArray args, String callbackId) {
		PluginResult.Status statusOK = PluginResult.Status.OK;
		PluginResult pres = null;
		try {
			if (action.equals("_FIND")) {
				pres = new PluginResult(statusOK, this.findSensors());
			} 
			
			else if (action.equals("_CONNECT")) {
				JSONObject obj = this.connectSensor(args.getString(0));
				if(obj != null) {
					pres = new PluginResult(statusOK, obj);
				}
				else pres = new PluginResult(PluginResult.Status.ERROR);
			} 
			
			else if (action.equals("_WATCH")) {
				Log.d("SensorsPlugin","Values: " + args.getString(0) + " - " + args.getInt(1));
				
				int handle = this.watchSensor(args.getString(0), args.getInt(1));
				
				callbacks.put(String.valueOf(handle), callbackId);
				pres = new PluginResult(statusOK,new JSONObject("{handle: " + handle + "}"));
				pres.setKeepCallback(true);
			} 
			
			else if (action.equals("_END_WATCH")) {
				this.endWatch(args.getInt(0));
				this.sendUpdate(callbacks.get(String.valueOf(args.getInt(0))));
				callbacks.remove(String.valueOf(args.getInt(0)));
				pres = new PluginResult(statusOK);
			} 
			
			else {
				pres = new PluginResult(PluginResult.Status.INVALID_ACTION,
						"Sensors: Invalid action: " + action);
			}
			return pres;
		} catch (JSONException e) {
			Log.e("SensorsPlugin",e.getMessage(),e);
			return new PluginResult(PluginResult.Status.JSON_EXCEPTION);
		}
	}


	/**
	 *  Sensor List
	 * 
	 * @return
	 * @throws JSONException
	 */
	private JSONArray findSensors() throws JSONException {
		String sensors = _native.listSensors();

		JSONArray arr = new JSONArray(sensors);

		return arr;
	}

	/**
	 *  Connection to the sensor
	 * 
	 * @param type
	 * @return
	 * @throws JSONException
	 */
	private JSONObject connectSensor(String type) throws JSONException {
		String res = _native.connect(type);
		JSONObject obj = null;

		if(!res.equals("-1")) {
			obj = new JSONObject(res);
		}
		
		return obj;
	}

	/**
	 *  Watch sensor
	 * 
	 * @param handle
	 * @param interval
	 * @throws JSONException
	 */
	private int watchSensor(String type, int interval) throws JSONException {
		return _native.watch(type, interval);
	}

	/**
	 *  End watch
	 * 
	 * @param handle
	 * @throws JSONException
	 */
	private void endWatch(int handle) throws JSONException {
		_native.kill(handle);
	}

	/**
	 *  The callback related to startWatch is finally closed
	 * 
	 * @param callbackId
	 * @throws JSONException
	 */
	private void sendUpdate(String callbackId) throws JSONException {
		JSONObject obj = new JSONObject("{end: true}");
		
		this.success(new PluginResult(PluginResult.Status.OK,obj), callbackId);
	}
	/**
	 *  Invoked when a new event coming from a sensor has been received from the native layer
	 *  
	 *  setKeepCallback is set to true as it is expected that more events will come afterwards
	 * 
	 * 
	 * @param handle
	 * @param event
	 * @throws JSONException
	 */
	public void sensorEvent(int handle,String event) throws JSONException {
		
		Log.d("SensorsPlugin", event);
		
		JSONObject obj = new JSONObject(event);
		
		PluginResult result = new PluginResult(PluginResult.Status.OK,obj);
		result.setKeepCallback(true);
		
		this.success(result, callbacks.get(String.valueOf(handle)));
	}
}