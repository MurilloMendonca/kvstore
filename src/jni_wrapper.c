#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string.h>

#include "mapper.h"

static const char *JNIT_CLASS = "MapperWrapper";

static jint c_init_map(JNIEnv *env, jobject thiz) {
    return init_map();
}
static jint c_set_val(JNIEnv *env, jobject thiz, jint map_id, jbyteArray key, jbyteArray value) {
    jsize key_len = (*env)->GetArrayLength(env, key);
    jsize value_len = (*env)->GetArrayLength(env, value);
    char *c_key = (char *)malloc(key_len);
    char *c_value = (char *)malloc(value_len);
    (*env)->GetByteArrayRegion(env, key, 0, key_len, c_key);
    (*env)->GetByteArrayRegion(env, value, 0, value_len, c_value);
    int ret = set_val(map_id, c_key, key_len, c_value, value_len);
    free(c_key);
    free(c_value);
    return ret;
}

static jbyteArray c_get_val(JNIEnv *env, jobject thiz, jint map_id, jbyteArray key) {
    jsize key_len = (*env)->GetArrayLength(env,key);
    char *c_key = (char *)malloc(key_len);
    (*env)->GetByteArrayRegion(env, key, 0, key_len, c_key);
    char *c_value = (char *)malloc(1024);
    int ret = get_val(map_id, c_key, key_len, c_value);
    free(c_key);
    if (ret == -1) {
        free(c_value);
        return NULL;
    }
    jbyteArray value = (*env)->NewByteArray(env, ret);
    (*env)->SetByteArrayRegion(env, value, 0, ret, c_value);
    free(c_value);
    return value;
    
}

static void c_destroy_map(JNIEnv *env, jobject thiz, jint map_id) {
    destroy_map(map_id);
}

static void c_destroy_all_maps(JNIEnv *env, jobject thiz) {
    destroy_all_maps();
}


static JNINativeMethod funcs[] = {
	{ "c_get_val", "(I[B)[B", (void *)&c_get_val },
    { "c_set_val", "(I[B[B)I", (void *)&c_set_val },
    { "c_init_map", "()I", (void *)&c_init_map },
    { "c_destroy_map", "(I)V", (void *)&c_destroy_map },
    { "c_destroy_all_maps", "()V", (void *)&c_destroy_all_maps }
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	jclass  cls;
	jint    res;

	(void)reserved;

	if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK)
		return -1;

	cls = (*env)->FindClass(env, JNIT_CLASS);
	if (cls == NULL)
		return -1;

	res = (*env)->RegisterNatives(env, cls, funcs, sizeof(funcs)/sizeof(*funcs));
	if (res != 0)
		return -1;

	return JNI_VERSION_1_8;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
	jclass  cls;

	(void)reserved;

	if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK)
		return;

	cls = (*env)->FindClass(env, JNIT_CLASS);
	if (cls == NULL)
		return;

	(*env)->UnregisterNatives(env, cls);
}

