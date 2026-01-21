/*
 * Open Surge Engine
 * androidplatform.c - scripting system: Android-specific routines
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <surgescript.h>

#if defined(__ANDROID__)
#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#endif

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_sharetext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);



/*
 * scripting_register_androidplatform()
 * Register the AndroidPlatform object
 */
void scripting_register_androidplatform(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "AndroidPlatform", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "AndroidPlatform", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "AndroidPlatform", "spawn", fun_spawn, 1);

    surgescript_vm_bind(vm, "AndroidPlatform", "shareText", fun_sharetext, 1);
}




/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* share a plain text using the Android Sharesheet */
surgescript_var_t* fun_sharetext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(__ANDROID__)

    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* text = surgescript_var_get_string(param[0], manager);

    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "shareText", "(Ljava/lang/String;)V");

    jstring jtext = (*env)->NewStringUTF(env, text);
    (*env)->CallVoidMethod(env, activity, method_id, jtext);
    (*env)->DeleteLocalRef(env, jtext);

    (*env)->DeleteLocalRef(env, class_id);

    ssfree(text);
    return NULL;

#else

    /* do nothing if the engine is not running on Android */
    return NULL;

#endif
}