#pragma once
#include <jni.h>
#include <jvmti.h>
#include <string>
#include "offline_profiles.hpp"

namespace OriginalSession {
inline jobject session=nullptr, owner=nullptr;
inline jfieldID field=nullptr;
inline void Reset(JNIEnv* env){if(env){if(session)env->DeleteGlobalRef(session);if(owner)env->DeleteGlobalRef(owner);}session=nullptr;owner=nullptr;field=nullptr;}
}
inline bool RestoreOriginalSession(JNIEnv* env,jclass minecraft,jfieldID instance,jfieldID world,std::string& message){
 if(!env||!minecraft||!instance||!world||!OriginalSession::session||!OriginalSession::owner||!OriginalSession::field){message="Original profile is unavailable in this session.";return false;}
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(env->ExceptionCheck()||!mc){if(env->ExceptionCheck())env->ExceptionClear();message="Minecraft is unavailable.";return false;}
 jobject w=env->GetObjectField(mc,world);if(env->ExceptionCheck()){env->ExceptionClear();env->DeleteLocalRef(mc);message="Could not check the current world.";return false;}
 if(w){env->DeleteLocalRef(w);env->DeleteLocalRef(mc);message="Leave the world before changing profiles.";return false;}
 if(!env->IsSameObject(mc,OriginalSession::owner)){env->DeleteLocalRef(mc);message="The original session does not match the current Minecraft instance.";return false;}
 env->SetObjectField(mc,OriginalSession::field,OriginalSession::session);const bool ok=!env->ExceptionCheck();if(!ok)env->ExceptionClear();env->DeleteLocalRef(mc);message=ok?"Original profile restored.":"Could not restore the original profile.";return ok;
}
inline bool ApplyOfflineProfile(JavaVM* vm, JNIEnv* env, jclass minecraft, jfieldID instance, jfieldID world, const std::string& name, std::string& message){
 if(!vm||!env||!minecraft||!instance||!world){message="Minecraft has not been recognized yet.";return false;}
 if(!OfflineProfiles::Valid(name)){message="Invalid nickname.";return false;}
 jvmtiEnv* ti=nullptr;if(vm->GetEnv(reinterpret_cast<void**>(&ti),JVMTI_VERSION_1_2)!=JNI_OK||!ti){message="JVMTI is unavailable.";return false;}
 if(env->PushLocalFrame(32)!=JNI_OK){message="Not enough memory to change profile.";return false;}
 bool ok=false;
 do {
  jobject mc=env->GetStaticObjectField(minecraft,instance);if(env->ExceptionCheck()||!mc){message="Minecraft is unavailable.";break;}
  jobject currentWorld=env->GetObjectField(mc,world);if(env->ExceptionCheck()){message="Could not check the current world.";break;}if(currentWorld){message="Leave the world before changing profiles.";break;}
  jint count=0;jfieldID* fields=nullptr;if(ti->GetClassFields(minecraft,&count,&fields)!=JVMTI_ERROR_NONE||!fields){message="Could not identify the session.";break;}
  jfieldID sessionField=nullptr;jclass sessionClass=nullptr;jmethodID ctor=nullptr;int matches=0;
  for(jint i=0;i<count;i++){
   jint flags=0;if(ti->GetFieldModifiers(minecraft,fields[i],&flags)!=JVMTI_ERROR_NONE||(flags&8))continue;
   char* sig=nullptr;if(ti->GetFieldName(minecraft,fields[i],nullptr,&sig,nullptr)!=JVMTI_ERROR_NONE||!sig)continue;
   bool object=sig[0]=='L';ti->Deallocate(reinterpret_cast<unsigned char*>(sig));if(!object)continue;
   jobject value=env->GetObjectField(mc,fields[i]);if(env->ExceptionCheck()){env->ExceptionClear();continue;}if(!value)continue;
   jclass cls=env->GetObjectClass(value);env->DeleteLocalRef(value);if(!cls||env->ExceptionCheck()){env->ExceptionClear();continue;}
   jmethodID c=env->GetMethodID(cls,"<init>","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");if(env->ExceptionCheck())env->ExceptionClear();
   if(c){++matches;if(matches==1){sessionField=fields[i];sessionClass=cls;ctor=c;continue;}}
   env->DeleteLocalRef(cls);
  }
  ti->Deallocate(reinterpret_cast<unsigned char*>(fields));
  if(matches!=1){message="Session could not be safely identified in this client.";break;}
  jclass uuidClass=env->FindClass("java/util/UUID");if(env->ExceptionCheck()||!uuidClass){message="UUID is unavailable.";break;}
  jmethodID fromBytes=env->GetStaticMethodID(uuidClass,"nameUUIDFromBytes","([B)Ljava/util/UUID;");jmethodID toString=env->GetMethodID(uuidClass,"toString","()Ljava/lang/String;");if(env->ExceptionCheck()||!fromBytes||!toString){message="UUID is unavailable.";break;}
  const std::string raw="OfflinePlayer:"+name;jbyteArray bytes=env->NewByteArray(static_cast<jsize>(raw.size()));if(!bytes||env->ExceptionCheck()){message="Could not generate UUID.";break;}env->SetByteArrayRegion(bytes,0,static_cast<jsize>(raw.size()),reinterpret_cast<const jbyte*>(raw.data()));
  jobject uuid=env->CallStaticObjectMethod(uuidClass,fromBytes,bytes);if(env->ExceptionCheck()||!uuid){message="Could not generate UUID.";break;}
  auto id=static_cast<jstring>(env->CallObjectMethod(uuid,toString));if(env->ExceptionCheck()||!id){message="Could not generate UUID.";break;}
  jstring user=env->NewStringUTF(name.c_str()),token=env->NewStringUTF("0"),type=env->NewStringUTF("legacy");if(env->ExceptionCheck()||!user||!token||!type){message="Could not create session.";break;}
  jobject session=env->NewObject(sessionClass,ctor,user,id,token,type);if(env->ExceptionCheck()||!session){message="Could not create offline session.";break;}
  if(!OriginalSession::session){
   jobject oldSession=env->GetObjectField(mc,sessionField);if(env->ExceptionCheck()||!oldSession){message="Original session is unavailable.";break;}
   OriginalSession::session=env->NewGlobalRef(oldSession);OriginalSession::owner=env->NewGlobalRef(mc);OriginalSession::field=sessionField;
   if(env->ExceptionCheck()||!OriginalSession::session||!OriginalSession::owner){if(env->ExceptionCheck())env->ExceptionClear();OriginalSession::Reset(env);message="Could not save the original session.";break;}
  }else if(!env->IsSameObject(mc,OriginalSession::owner)||sessionField!=OriginalSession::field){message="Session changed; restart Boull.";break;}
  env->SetObjectField(mc,sessionField,session);if(env->ExceptionCheck()){message="Could not apply session.";break;}
  message="Offline profile applied: "+name;ok=true;
 }while(false);
 if(env->ExceptionCheck())env->ExceptionClear();env->PopLocalFrame(nullptr);return ok;
}


