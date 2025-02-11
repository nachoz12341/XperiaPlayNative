/*
 * Kwaak3 - Java to quake3 interface
 * Copyright (C) 2010 Roderick Colenbrander
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>

#include "org_zeus_arena_KwaakJNI.h"

/* Function pointers to Quake3 code */
int  (*q3main)(int argc, char **argv);
void (*drawFrame)();
void (*queueKeyEvent)(int key, int state);
void (*queueMotionEvent)(int action, float x, float y, float pressure);
void (*queueTouchEvent)(int action, float x, float y, float pressure);
void (*queueTrackballEvent)(int action, float x, float y);
void (*requestAudioData)();
void (*setAudioCallbacks)(void *func, void *func2, void *func3);
void (*setResolution)(int width, int height);
void (*setInputCallbacks)(void *func);

/* Callbacks to Android */
jmethodID android_getPos;
jmethodID android_initAudio;
jmethodID android_writeAudio;
jmethodID android_setMenuState;

/* Containts the path to /data/data/(package_name)/libs */
static char* lib_dir=NULL;

static JavaVM *jVM;
static jboolean audioEnabled=1;
static jboolean benchmarkEnabled=0;
static jboolean lightmapsEnabled=0;
static jboolean pureServersEnabled=0;
static jboolean openArenaProtocolEnabled=0;
static jboolean showFramerateEnabled=0;
static jobject audioBuffer=0;
static jobject kwaakAudioObj=0;
static jobject kwaakRendererObj=0;
static void *libdl;
static int init=0;

typedef unsigned char BOOL;
#define FALSE 0
#define TRUE 1

//#define DEBUG

static BOOL neon_support()
{
    char buf[80];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(!fp)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "Quake", "Unable to open /proc/cpuinfo\n");
        return FALSE;
    }

    while(fgets(buf, 80, fp) != NULL)
    {
        char *features = strstr(buf, "Features");

        if(features)
        {
            char *feature;
            features += strlen("Features");
            feature = strtok(features, ": ");
            while(feature)
            {
                if(!strcmp(feature, "neon"))
                    return TRUE;

                feature = strtok(NULL, ": ");
            }
            return FALSE;
        }
    }
    return FALSE;
}

const char *get_quake3_library()
{
    /* We ship a library with Neon FPU support. This boosts performance a lot but it only works on a few CPUs. */
    if(neon_support())
        return "libquake3_neon.so";

    return "libquake3.so";
}

void get_quake3_library_path(char *path)
{
    const char *libquake3 = get_quake3_library();
    if(lib_dir)
    {
        sprintf(path, "%s/%s", lib_dir, libquake3);
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "Library path not set, trying /data/data/org.zeus.arena/lib");
        sprintf(path, "/data/data/org.zeus.arena/lib/%s", libquake3);
    }
}

static void load_libquake3()
{
    char libquake3_path[80];
    get_quake3_library_path(libquake3_path);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake", "Attempting to load %s\n", libquake3_path);
#endif

    libdl = dlopen(libquake3_path, RTLD_NOW);
    if(!libdl)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake", "Unable to load libquake3.so: %s\n", dlerror());
        return;
    }

    q3main = dlsym(libdl, "main");
    drawFrame = dlsym(libdl, "nextFrame");
    queueKeyEvent = dlsym(libdl, "queueKeyEvent");
    queueMotionEvent = dlsym(libdl, "queueMotionEvent");
    queueTrackballEvent = dlsym(libdl, "queueTrackballEvent");
    requestAudioData = dlsym(libdl, "requestAudioData");
    setAudioCallbacks = dlsym(libdl, "setAudioCallbacks");
    setResolution = dlsym(libdl, "setResolution");
    setInputCallbacks = dlsym(libdl, "setInputCallbacks");
    queueTouchEvent = dlsym(libdl,"queueTouchEvent");
    init=1;
}

int getPos()
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "getPos");
#endif
    return (*env)->CallIntMethod(env, kwaakAudioObj, android_getPos);
}

void initAudio(void *buffer, int size)
{
    JNIEnv *env;
    jobject tmp;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initAudio");
#endif
    tmp = (*env)->NewDirectByteBuffer(env, buffer, size);
    audioBuffer = (jobject)(*env)->NewGlobalRef(env, tmp);

    if(!audioBuffer) __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "yikes, unable to initialize audio buffer");

    return (*env)->CallVoidMethod(env, kwaakAudioObj, android_initAudio);
}

void writeAudio(int offset, int length)
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "writeAudio audioBuffer=%p offset=%d length=%d", audioBuffer, offset, length);
#endif

    (*env)->CallVoidMethod(env, kwaakAudioObj, android_writeAudio, audioBuffer, offset, length);
}


int JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    jVM = vm;

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "JNI_OnLoad called");
#endif

    if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "Quake_JNI", "Failed to get the environment using GetEnv()");
        return -1;
    }

    if(!init) load_libquake3();

    return JNI_VERSION_1_4;
}

void setMenuState(int state){
	JNIEnv *env;
	(*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
	#ifdef DEBUG
		__android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "setMenuState state=%d", state);
	#endif

    (*env)->CallVoidMethod(env, kwaakRendererObj, android_setMenuState, state);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_enableAudio(JNIEnv *env, jclass c, jboolean enable)
{
    audioEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_enableBenchmark(JNIEnv *env, jclass c, jboolean enable)
{
    benchmarkEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_enableLightmaps(JNIEnv *env, jclass c, jboolean enable)
{
    lightmapsEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_enablePureServers(JNIEnv *env, jclass c, jboolean enable){
    pureServersEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_enableOpenArenaProtocol(JNIEnv *env, jclass c, jboolean enable){
	openArenaProtocolEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_setRenderer(JNIEnv *env, jclass c, jobject obj){
    kwaakRendererObj = obj;
    jclass kwaakRendererClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakRendererObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakRendererClass = (*env)->GetObjectClass(env, kwaakRendererObj);

	android_setMenuState = (*env)->GetMethodID(env,kwaakRendererClass,"setMenuState","(I)V");
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_showFramerate(JNIEnv *env, jclass c, jboolean enable)
{
    showFramerateEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_setAudio(JNIEnv *env, jclass c, jobject obj)
{
    kwaakAudioObj = obj;
    jclass kwaakAudioClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakAudioObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakAudioClass = (*env)->GetObjectClass(env, kwaakAudioObj);

    android_getPos = (*env)->GetMethodID(env,kwaakAudioClass,"getPos","()I");
    android_initAudio = (*env)->GetMethodID(env,kwaakAudioClass,"initAudio","()V");
    android_writeAudio = (*env)->GetMethodID(env,kwaakAudioClass,"writeAudio","(Ljava/nio/ByteBuffer;II)V");
}


JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_initGame(JNIEnv *env, jclass c, jint width, jint height)
{
	__android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initGame(%d, %d) 1", width, height);
    char *argv[6];
    int argc=0;

    /* TODO: integrate settings with quake3, right now there is no synchronization */

    if(!audioEnabled)
    {
        argv[argc] = strdup("+set s_initsound 0");
        argc++;
    }

    if(lightmapsEnabled)
        argv[argc] = strdup("+set r_vertexlight 0");
    else
        argv[argc] = strdup("+set r_vertexlight 1");
    argc++;

    if(showFramerateEnabled)
        argv[argc] = strdup("+set cg_drawfps 1");
    else
        argv[argc] = strdup("+set cg_drawfps 0");
    argc++;

    if(pureServersEnabled){
      	argv[argc] = strdup("+set sv_pure 1");
    }
    else{
      	argv[argc] = strdup("+set sv_pure 0");
    }
    argc++;

    if (openArenaProtocolEnabled){
    	argv[argc] = strdup("+set protocol 71");
    	argc++;
    }

    if(benchmarkEnabled)
    {
        argv[argc] = strdup("+demo four +timedemo 1");
        argc++;
    }

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initGame(%d, %d)", width, height);
#endif

    setAudioCallbacks(&getPos, &writeAudio, &initAudio);
    setInputCallbacks(&setMenuState);
    setResolution(width, height);
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initGame(%d, %d) 2", width, height);

    /* In the future we might want to pass arguments using argc/argv e.g. to start a benchmark at startup, to load a mod or whatever */
    q3main(argc, argv);
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "initGame(%d, %d) 3", width, height);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_drawFrame(JNIEnv *env, jclass c)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "nextFrame()");
#endif
    if(drawFrame) drawFrame();
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_queueKeyEvent(JNIEnv *env, jclass c, jint key, jint state)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueKeyEvent(%d, %d)", key, state);
#endif
    if(queueKeyEvent) queueKeyEvent(key, state);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_queueMotionEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y, jfloat pressure)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueMotionEvent(%d, %f, %f, %f)", action, x, y, pressure);
#endif
    if(queueMotionEvent) queueMotionEvent(action, x, y, pressure);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_queueTouchEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y, jfloat pressure)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueTouchEvent(%d, %f, %f, %f)", action, x, y, pressure);
#endif
    if(queueTouchEvent) queueTouchEvent(action, x, y, pressure);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_queueTrackballEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y)
{
#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "queueTrackballEvent(%d, %f, %f)", action, x, y);
#endif
    if(queueTrackballEvent) queueTrackballEvent(action, x, y);
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_requestAudioData(JNIEnv *env, jclass c)
{
    if(requestAudioData) requestAudioData();
}

JNIEXPORT void JNICALL Java_org_zeus_arena_KwaakJNI_setLibraryDirectory(JNIEnv *env, jclass c, jstring jpath)
{
    jboolean iscopy;
    const jbyte *path = (*env)->GetStringUTFChars(env, jpath, &iscopy);
    lib_dir = strdup(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);

#ifdef DEBUG
    __android_log_print(ANDROID_LOG_DEBUG, "Quake_JNI", "path=%s\n", lib_dir);
#endif
}
