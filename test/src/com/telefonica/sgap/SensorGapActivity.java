package com.telefonica.sgap;

import com.phonegap.*;
import android.os.Bundle;

public class SensorGapActivity extends DroidGap {
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		super.loadUrl("file:///android_asset/www/index2.html");
	}
	
	static {
    	System.loadLibrary("native-sensors");
    }
}