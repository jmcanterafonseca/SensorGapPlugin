package com.telefonica.sensors;

import android.util.Log;

public class SensorNative {
	
	public native String connect(String uri);
	
	public native int watch(String type,int interval);
	
	public native void end(int handle);
	
	public native void kill(int handle);
	
	public native void killAll();
	
	public native String listSensors();
}