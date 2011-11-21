/**
 *  Glue code for sensors
 *
 *
 */

#include <jni.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android/looper.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "sensors-library", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "sensors-library", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "sensors-library", __VA_ARGS__))

static pthread_mutex_t list_mutex;

static JavaVM* cached_jvm;
static jclass class_SensorImpl = NULL;
static jmethodID meth_sensorData = NULL;

#define S_ACCEL "Accelerometer"
#define S_GYR_ "Gyroscope"
#define S_TEMP "Temperature"
#define S_AMB_LIGHT "AmbientLight"
#define S_AMB_NOISE "AmbientNoise"
#define S_MAG_FIELD "MagneticField"
#define S_ATM_PRESSURE "AtmPressure"
#define S_PROXIMITY "Proximity"
#define S_ORIENTATION "Orientation"
#define S_RELATIVE_HUMIDITY "RelHumidity"
#define S_GRAVITY "Gravity"
#define S_LIN_ACCEL "LinearAcceleration"
#define S_ROT_VECT "RotationVector"
#define UNKNOWN "unknown"

#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_ORIENTATION         3
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PRESSURE            6
#define SENSOR_TYPE_TEMPERATURE         7
#define SENSOR_TYPE_PROXIMITY           8
#define SENSOR_TYPE_GRAVITY             9
#define SENSOR_TYPE_LINEAR_ACCELERATION 10
#define SENSOR_TYPE_ROTATION_VECTOR     11
#define SENSOR_TYPE_RELATIVE_HUMIDITY   12


/**
 *  Prepares all the resources needed for the library
 *
 *
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void *reserved) {
	JNIEnv *env;
	jclass clsSensorImpl;
	jclass clsSensorEvent;

	cached_jvm = jvm; /* cache the JavaVM pointer */

	if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_2)) {
		return JNI_ERR; /* JNI version not supported */
	}

	clsSensorImpl = (*env)->FindClass(env, "com/telefonica/sensors/SensorNativeCBridge");
	if (clsSensorImpl == NULL) {
		return JNI_ERR;
	}

	// Use weak global ref to allow C class to be unloaded
	class_SensorImpl = (*env)->NewWeakGlobalRef(env, clsSensorImpl);
	if (class_SensorImpl == NULL) {
		return JNI_ERR;
	}

	meth_sensorData = (*env)->GetStaticMethodID(env, clsSensorImpl,
			"sensorDataCB", "(ILjava/lang/String;)V");
	if (meth_sensorData == NULL) {
		return JNI_ERR;
	}

	pthread_mutex_init(&list_mutex, NULL);

	return JNI_VERSION_1_2;
}



typedef struct SensorCnxData SensorCnxData;

struct SensorCnxData {
	ASensorRef sensor;
	ALooper* looper;
	const char* status;
	int interval;
	int hasToFinish;
	// int handle;
};

typedef struct SensorThreadData SensorThreadData;

struct SensorThreadData {
	SensorCnxData* sdata;
	jobject target;
	int handle;
};

typedef struct SensorCnxListNode SensorCnxListNode;

struct SensorCnxListNode {
	SensorCnxListNode* prev;
	SensorCnxListNode* next;
	SensorCnxData* data;
};

// Pointers to the double linked list with the handlers
static SensorCnxListNode* firstCnxNode = NULL;

/**
 *
 *  Adds a node to the double linked list
 *
 */
static void addNode(SensorCnxListNode* node) {
	pthread_mutex_lock(&list_mutex);

	if (firstCnxNode == NULL) {
		firstCnxNode = node;
		firstCnxNode->prev = NULL;
		firstCnxNode->next = NULL;
	} else {
		SensorCnxListNode* aux = node;
		aux->next = node;
		node->prev = aux;
		node->next = NULL;
	}

	pthread_mutex_unlock(&list_mutex);
}

/**
 *   Removes a node from the double linked list
 *
 */
static void removeNode(SensorCnxListNode* node) {

	pthread_mutex_lock(&list_mutex);

	SensorCnxListNode* prev = node->prev;
	SensorCnxListNode* next = node->next;

	if (node == firstCnxNode) {
		firstCnxNode = node->next;
		if (firstCnxNode != NULL) {
			firstCnxNode->prev = NULL;
		}
	} else if (next != NULL && prev != NULL) {
		next->prev = node->prev;
		prev->next = node->next;
	} else if (next == NULL) {
		// Last node to be removed
		prev->next = NULL;
	}

	free(node);

	pthread_mutex_unlock(&list_mutex);
}

/**
 *  Obtains a sensor type as a number from a sensor type as a string
 *
 *
 */
static int getSensorTypeAsInt(const char* uri) {
	int dev = -1;

	if (strcmp(S_ACCEL, uri) == 0) {
		dev = ASENSOR_TYPE_ACCELEROMETER;
	} else if (strcmp(S_AMB_LIGHT, uri) == 0) {
		dev = ASENSOR_TYPE_LIGHT;
	} else if (strcmp(S_GYR_, uri)== 0) {
		dev = ASENSOR_TYPE_GYROSCOPE;
	} else if (strcmp(S_PROXIMITY, uri)== 0) {
		dev = ASENSOR_TYPE_PROXIMITY;
	} else if (strcmp(S_MAG_FIELD, uri)== 0) {
		dev = ASENSOR_TYPE_MAGNETIC_FIELD;
	}
	else if (strcmp(S_TEMP, uri)== 0) {
		dev = SENSOR_TYPE_TEMPERATURE;
	}
	else if (strcmp(S_ORIENTATION, uri)== 0) {
		dev = SENSOR_TYPE_ORIENTATION;
	}
	else if (strcmp(S_GRAVITY, uri)== 0) {
		dev = SENSOR_TYPE_GRAVITY;
	}
	else if (strcmp(S_LIN_ACCEL, uri)== 0) {
		dev = SENSOR_TYPE_LINEAR_ACCELERATION;
	}
	else if (strcmp(S_RELATIVE_HUMIDITY, uri)== 0) {
		dev = SENSOR_TYPE_RELATIVE_HUMIDITY;
	}
	else if (strcmp(S_ROT_VECT, uri)== 0) {
			dev = SENSOR_TYPE_ROTATION_VECTOR;
	}

	return dev;
}

/*
#define SENSOR_TYPE_ACCELEROMETER       1
#define SENSOR_TYPE_MAGNETIC_FIELD      2
#define SENSOR_TYPE_ORIENTATION         3
#define SENSOR_TYPE_GYROSCOPE           4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PRESSURE            6
#define SENSOR_TYPE_TEMPERATURE         7
#define SENSOR_TYPE_PROXIMITY           8
#define SENSOR_TYPE_GRAVITY             9
#define SENSOR_TYPE_LINEAR_ACCELERATION 10
#define SENSOR_TYPE_ROTATION_VECTOR     11
#define SENSOR_TYPE_RELATIVE_HUMIDITY   12 */

/**
 *   Obtains a sensor type as a string from a sensor type as a number
 *
 */
static const char* getSensorTypeAsString(int type) {
	static const char* stypes[] = { NULL, S_ACCEL, S_MAG_FIELD, S_ORIENTATION,
	S_GYR_, S_AMB_LIGHT, S_ATM_PRESSURE, S_TEMP, S_PROXIMITY,S_GRAVITY,S_LIN_ACCEL,S_ROT_VECT,S_RELATIVE_HUMIDITY };
	static const char* unk = UNKNOWN;
	const char* ret = unk;

	if (type <= 12) {
		ret = stypes[type];
	}

	if (ret == NULL) {
		ret = unk;
	}

	return ret;
}

/**
 *  Convenience function for the conversion between a handle and a node
 *
 */
static SensorCnxListNode* handle2Node(int handle) {
	SensorCnxListNode* ret = (SensorCnxListNode*) handle;

	return ret;
}

/**
 *  Convenience function for the conversion between a node and a handle
 *
 */
static int node2Handle(SensorCnxListNode* node) {
	return (int) node;
}

/**
 *   Returns connection data about a sensor
 *
 */
static SensorCnxData* getCnxData(int handle) {
	SensorCnxData* ret = handle2Node(handle)->data;

	return ret;
}

/**
 *  Given a handle it returns the corresponding sensor reference
 *
 *
 */
static ASensorRef getSensor(int handle) {
	ASensorRef sensor = getCnxData(handle)->sensor;

	LOGI("Sensor Got: %d", sensor);

	return sensor;
}

/**
 *   Adds a sensor to the linked list and returns a handle
 *
 *
 */
static int addSensor(ASensorRef sensor) {

	SensorCnxListNode* node = (SensorCnxListNode*) malloc(
			sizeof(SensorCnxListNode));

	node->data = (SensorCnxData*) malloc(sizeof(SensorCnxData));

	node->data->sensor = sensor;

	addNode(node);

	LOGI("Sensor Added: %d", sensor);

	// node->data->handle = (int) node;
	node->data->status = "open";

	return (int)node;
}

/**
 *  Serialize sensor metadata in JSON format
 *
 *
 */
static char* getSensorMetadata(ASensorRef sensor) {
	char* ret = malloc(512);

	bzero(ret,sizeof(ret));

	const char* vendor = ASensor_getVendor(sensor);
	const char* type = getSensorTypeAsString(ASensor_getType(sensor));
	const char* name = ASensor_getName(sensor);
	int minDelay = ASensor_getMinDelay(sensor) / 1000;
	float resolution = ASensor_getResolution(sensor);

	sprintf(ret,"{vendor:'%s',type:'%s',id:'%s',minDelay: %d, resolution: %f}",
											vendor,type,name,minDelay,resolution);

	return ret;
}

/**
 *   'Connects' the sensor and gets all the metadata associated to it
 *
 *
 */
JNIEXPORT jstring JNICALL Java_com_telefonica_sensors_SensorNative_connect(
		JNIEnv* env, jobject thiz, jstring type) {
	ASensorManager* sensorManager;
	ASensorRef sensor;

	jboolean isCopy;
	const char* suri = (*env)->GetStringUTFChars(env, type, &isCopy);

	LOGI("Connect Requestx for type: %s", suri);

	sensorManager = ASensorManager_getInstance();

	int itype = getSensorTypeAsInt(suri);
	char* aux;
	char* output = NULL;
	const char* error = "-1";

	if(itype != -1) {
		sensor = ASensorManager_getDefaultSensor(sensorManager,itype);
		if(sensor != NULL) {
			output = getSensorMetadata(sensor);
			aux = output;
		}
		else {
			aux = error;
		}
	}
	else {
			aux = error;
	}

	jstring result = (*env)->NewStringUTF(env, aux);

	LOGI("Returned metadata: %s",output);

	if(output != NULL) {
		free(output);
	}

	return result;
}


/**
 * Callback function called new sensor data is available
 *
 */
static int sensorCB(int fd, int events, void* data) {
	/*
	 LOGI("The callback has been invoked: events: %d fd: %d\n");

	 ASensorEvent* event = (ASensorEvent*)data;

	 LOGI("yyyyy Events received: %d %d %d\n",event->acceleration.x, event->acceleration.y,event->acceleration.z);
	 */
}

/**
 *   Serializes to JSON a sensor event
 *
 *
 */
static char* serializeEvent(ASensorEvent asv,double t,int type) {
	char* ret = malloc(256);
	bzero(ret, sizeof(ret));

	switch(type) {
		case SENSOR_TYPE_ACCELEROMETER:
		case SENSOR_TYPE_GRAVITY:
		case SENSOR_TYPE_ROTATION_VECTOR:
		case SENSOR_TYPE_LINEAR_ACCELERATION:
		case SENSOR_TYPE_GYROSCOPE:
		case SENSOR_TYPE_MAGNETIC_FIELD:
			sprintf(ret, "{data: {x:%f,y:%f,z:%f}", asv.data[0],
					asv.data[1], asv.data[2]);
		break;

		case SENSOR_TYPE_LIGHT:
		case SENSOR_TYPE_PROXIMITY:
		case SENSOR_TYPE_TEMPERATURE:
			sprintf(ret, "{data: %f", asv.data[0]);
		break;

		case SENSOR_TYPE_ORIENTATION:
			sprintf(ret, "{data: {alpha:%f,beta:%f,gamma:%f}", asv.data[0],
								asv.data[1], asv.data[2]);
		break;

		// case SENSOR_TYPE
	}

	// LOGI("Timestamp: %u",asv.timestamp);
	char aux[64];

	sprintf(aux,",timestamp:%lf}",t);
	strcat(ret,aux);

	return ret;
}

/**
 *  It is invoked to send back the sensor data to the Java Layer and
 *  from there to the Javascript layer
 *
 *
 */
static void on_sensor_data(ASensorEvent event, int handle, jobject target,double ms) {
	JNIEnv* env;

	(*cached_jvm)->AttachCurrentThread(cached_jvm, &env, NULL);

	jclass c = (*env)->GetObjectClass(env, target);

	// jmethodID mc = (*env)->GetMethodID(env, clsSensorEvent, "<init>", "()V");

	// jobject obj = (*env)->NewObject(env, clsSensorEvent, mc);

	ASensorRef sensor = getCnxData(handle)->sensor;

	jstring str = (*env)->NewStringUTF(env, serializeEvent(event,ms,ASensor_getType(sensor)));

	(*env)->CallStaticVoidMethod(env, c, meth_sensorData, handle, str);

	(*cached_jvm)->DetachCurrentThread(cached_jvm);
}

/* return current time in microseconds */
static double
now_microseconds(void)
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return (double)res.tv_sec * 1e6 + (double)res.tv_nsec/1e3;
}


/**
 *  This is the code executed by a thread in charge of watching a sensor
 *
 */
static void* watcher(void* param) {

	LOGI("Watch thread started");

	SensorThreadData* sthr = (SensorThreadData*) param;
	SensorCnxData* wd = sthr->sdata;

	wd->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

	ASensorEventQueue* the_queue = ASensorManager_createEventQueue(
			ASensorManager_getInstance(), wd->looper, ALOOPER_POLL_CALLBACK,
			sensorCB, NULL);

	LOGI("Interval: %d",wd->interval);

	ASensorEventQueue_setEventRate(the_queue, wd->sensor, (int32_t)(wd->interval * 1000L));

	ASensorEventQueue_enableSensor(the_queue, wd->sensor);

	ASensorEvent sensorEvents[1];

	int finish = 0;

	double oldtimestamp = 0;
	double timestamp;

	while (!finish) {
		int events;
		int fd;
		int pollRes = ALooper_pollOnce(-1, &events, &fd, (void**) NULL);
		if (pollRes == ALOOPER_POLL_CALLBACK) {
			LOGI("Num Events: %d FD: %d\n", events, fd);
			bzero(sensorEvents, sizeof(sensorEvents));
			ssize_t numEvents = ASensorEventQueue_getEvents(the_queue,
					sensorEvents, 1);
			LOGI("Num events got: %d", numEvents);
			if (numEvents > 0) {
				LOGI("yyy Events received: %f %f %f\n", sensorEvents[0].acceleration.x,
								sensorEvents[0].acceleration.y, sensorEvents[0].acceleration.z);
				if(oldtimestamp == 0) {
					oldtimestamp = sensorEvents[0].timestamp;
					timestamp = now_microseconds();
				}
				else {
						double newtimestamp = (double)sensorEvents[0].timestamp;
						double difference =  newtimestamp - oldtimestamp;
						timestamp = timestamp + (difference / 1e3);
						oldtimestamp = newtimestamp;
				}
				on_sensor_data(sensorEvents[0], sthr->handle, sthr->target,timestamp);
			}
		} else if (pollRes == ALOOPER_POLL_WAKE) {
			finish = 1;
		}

		if (wd->hasToFinish) {
			finish = 1;
		}
	}

	// Now it is time to release resources bound to the thread
	ASensorEventQueue_disableSensor(the_queue, wd->sensor);
	ASensorManager_destroyEventQueue(ASensorManager_getInstance(), the_queue);

	LOGI("Watch Thread finished");
}

/**
 *  Implements the startWatch functionality at the native layer
 *
 *
 */
JNIEXPORT jint JNICALL Java_com_telefonica_sensors_SensorNative_watch(JNIEnv* env, jobject thiz,
		jstring type, jint interval) {

	LOGI("Watch invoked at the native layer");

	// We get the sensor add it to list
	ASensorManager* sensorManager = ASensorManager_getInstance();

	jboolean isCopy;
	const char* stype = (*env)->GetStringUTFChars(env, type, &isCopy);

	ASensorRef sensor = ASensorManager_getDefaultSensor(sensorManager,
				getSensorTypeAsInt(stype));

	int handle = addSensor(sensor);

	pthread_t theThread;

	SensorCnxData* wd = getCnxData(handle);
	wd->interval = interval;

	SensorThreadData* sthr = malloc(sizeof(SensorThreadData));
	sthr->sdata = wd;
	sthr->target= thiz;
	sthr->handle = handle;

	wd->hasToFinish = 0;

	wd->status = "watching";

	int rc = pthread_create(&theThread, NULL, watcher, (void *) sthr);

	return handle;
}

static void doEnd(int handle) {
SensorCnxData* cd = getCnxData(handle);

	if (strcmp(cd->status, "watching") == 0) {
		cd->hasToFinish = 1;
		cd->status = "open";
		ALooper_wake(cd->looper);
	}
}

/**
 *  Implements the endWatch functionality at native layer
 *
 *
 */
JNIEXPORT void JNICALL Java_com_telefonica_sensors_SensorNative_end(JNIEnv* env,
	jobject thiz, jint handle) {

	LOGI("End Watch invoked. Handle: %d", handle);

	doEnd(handle);

}

static void doKill(int handle) {
	doEnd(handle);

	 // Remove the node
	SensorCnxListNode* node = handle2Node(handle);

	removeNode(node);
}

JNIEXPORT void JNICALL Java_com_telefonica_sensors_SensorNative_kill(JNIEnv* env,
	jobject thiz, jint handle) {

	LOGI("Kill invoked. Handle: %d", handle);

	doKill(handle);
}

static doKillAll() {
	SensorCnxListNode* node = firstCnxNode;
		while(firstCnxNode != NULL) {
			doKill(node2Handle(firstCnxNode));
		}
}

JNIEXPORT void JNICALL Java_com_telefonica_sensors_SensorNative_killAll(JNIEnv* env,
	jobject thiz) {

	LOGI("Kill All invoked");

	doKillAll();

}


/**
 *   Implements the list sensors functionality at the native layer
 *
 */
JNIEXPORT jstring JNICALL Java_com_telefonica_sensors_SensorNative_listSensors(
JNIEnv* env, jobject thiz) {

	LOGI("List sensors invokedd");


	ASensorList list;

	int num = ASensorManager_getSensorList(ASensorManager_getInstance(), &list);

	LOGI("Num Sensors Discovered: %d",num);

	int j;
	char* buffer = malloc(512);
	bzero(buffer,sizeof(buffer));
	strcat(buffer,"[");

	for (j = 0; j < num; j++) {
		ASensorRef sensor = list[j];
		int type = ASensor_getType(sensor);

		const char* id = ASensor_getName(sensor);
		const char* typeName = getSensorTypeAsString(type);

		LOGI("Type name: %s",typeName);

		strcat(buffer,"{type: '");
		strcat(buffer,typeName);
		strcat(buffer,"'");

		strcat(buffer,", id: '");
		strcat(buffer,id);
		strcat(buffer,"'}");

		if(j < num - 1) {
			strcat(buffer,",");
		}
	}

	strcat(buffer,"]");

	LOGI("List Sensors returned: %s",buffer);

	jstring result = (*env)->NewStringUTF(env, buffer);

	free(buffer);

	return result;
}


/**
 *  Clean resources when the library is unloaded
 *
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved) {
	doKillAll();
	pthread_mutex_destroy(&list_mutex);
}
