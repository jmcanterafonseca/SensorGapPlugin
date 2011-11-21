
if(!window.navigator.SensorConnection) {
	window.navigator.SensorConnection =  function(t) {
		var type = t;
		var connectionHandle;
		var that = this;
		
		window.console.log('On sensor Connection');
		
		setStatus("new");
		
		PhoneGap.exec(function(value) { window.console.log('Got metadata'); setMetadata(value); } ,
			function(e) { 
				setStatus("error"); 
				throw new Error("Instantiation Exception"); },
			"SensorsPlugin","_CONNECT",[type]);
		
		function setStatus(val) {
			that.status = val;
			if(that.onstatuschange) {
				that.onstatuschange();
			}
		}
		
		function setMetadata(metadata) {
			that.sensor = metadata;
			setStatus("connected");
		}
		
		this.startWatch = function(options) {
			if(this.status == "connected") {
				PhoneGap.exec(function(e) { 	
									if(e.handle) {
										setStatus("watching");
										connectionHandle = e.handle;
									}
									if(that.onsensordata && e && !e.end && !e.handle) { 
										that.onsensordata(e); 
									}
							},
					      	function() { setStatus("error"); },
								"SensorsPlugin",
								"_WATCH",[type,options.interval]);
			}
		}
		
		this.endWatch = function() {
			if(this.status == "watching") {
				PhoneGap.exec(function() { setStatus("connected")},function() { setStatus("error");},
							"SensorsPlugin","_END_WATCH",[connectionHandle]);
			}
		}
		
		setStatus("connected");
	}
}

if(!window.navigator.sensors) {
	window.navigator.sensors = new function() {
		this.list = function() {
			window.console.log("Listed");
			var request = new SensorRequest();
			
			window.setTimeout(function() {
				window.console.log("To CB");
				PhoneGap.exec(request.completed,   // Success callback
					request.rejected,          // Error callback
					'SensorsPlugin',     	   // Tell PhoneGap to run "SensoraPlugin" Plugin
					'_FIND', 		   // Tell plugin, which action we want to perform
					[]);       		   // No parameters
			},0);
			
			return request;
		}
	}
}

function SensorRequest() {
	var that = this;
	
	this.completed = function(res) {
	
		window.console.log("Completed!! " + res);
		
		that.result = res;
		
		if(that.onsuccess) {
			that.onsuccess();
		}
	}
	
	this.rejected = function (error) {
		window.console.log("Error!!");
		
		that.error = error;
		if(that.onerror) {
			that.onerror();
		}
	}
}

