#include "../src/runtime.cpp"
#include <iostream>
#include <filesystem>

int main(){
    wchar_t home[32768]{};char classes[32768]{};
    const DWORD hn=GetEnvironmentVariableW(L"JAVA_HOME",home,32768);
    const DWORD cn=GetEnvironmentVariableA("AUTOCLICKV_TEST_CLASSES",classes,32768);
    if(!hn||hn>=32768||!cn||cn>=32768){std::cerr<<"Missing test paths\n";return 1;}
    std::filesystem::path bin=std::filesystem::path(home)/L"jre"/L"bin";
    if(!std::filesystem::exists(bin/L"server"/L"jvm.dll"))bin=std::filesystem::path(home)/L"bin";
    if(!SetDllDirectoryW(bin.c_str()))return 2;
    HMODULE lib=LoadLibraryW((bin/L"server"/L"jvm.dll").c_str());if(!lib)return 3;
    using CreateVM=jint(JNICALL*)(JavaVM**,void**,void*);
    auto create=reinterpret_cast<CreateVM>(GetProcAddress(lib,"JNI_CreateJavaVM"));if(!create)return 4;
    std::string cp="-Djava.class.path="+std::string(classes);JavaVMOption opt{};opt.optionString=cp.data();
    JavaVMInitArgs args{};args.version=JNI_VERSION_1_8;args.nOptions=1;args.options=&opt;
    JavaVM* testVm{};JNIEnv* testEnv{};
    if(create(&testVm,reinterpret_cast<void**>(&testEnv),&args)!=JNI_OK)return 5;
    jclass cls=testEnv->FindClass("net/minecraft/client/Minecraft");if(!cls)return 6;
    auto staticId=testEnv->GetStaticFieldID(cls,"theMinecraft","Lnet/minecraft/client/Minecraft;");if(!staticId)return 7;
    jobject object=testEnv->GetStaticObjectField(cls,staticId);if(!object)return 8;
    if(!AttachJvm())return 9;
    if(TryMapping(cls,mappings[1])||testEnv->ExceptionCheck()){std::cerr<<"Invalid mapping was accepted or left an exception\n";return 9;}
    if(!FindMinecraft()){std::cerr<<"Mapping was not found\n";return 9;}
    if(!debugFpsField||testEnv->GetStaticIntField(cls,debugFpsField)!=144){std::cerr<<"Game FPS field was not mapped\n";return 25;}
    if(!IsWorldReady()){std::cerr<<"Expected an active world\n";return 22;}
    std::string altStatus;
    if(ApplyOfflineProfile(vm,env,minecraft,instance,world,"TestUser",altStatus)){std::cerr<<"Accepted change while world open\n";return 34;}
    if(ApplyOfflineProfile(vm,env,minecraft,instance,world,"bad!",altStatus)){std::cerr<<"Accepted invalid nickname\n";return 35;}
    jobject savedWorld=testEnv->GetObjectField(object,world);testEnv->SetObjectField(object,world,nullptr);
    if(!ApplyOfflineProfile(vm,env,minecraft,instance,world,"TestUser",altStatus)){std::cerr<<"Profile apply failed: "<<altStatus<<"\n";return 36;}
    jfieldID sid=testEnv->GetFieldID(cls,"session","Lnet/minecraft/util/Session;");jobject sess=testEnv->GetObjectField(object,sid);jclass sc=testEnv->GetObjectClass(sess);jfieldID uname=testEnv->GetFieldID(sc,"username","Ljava/lang/String;");auto jname=static_cast<jstring>(testEnv->GetObjectField(sess,uname));const char* chars=testEnv->GetStringUTFChars(jname,nullptr);bool good=std::string(chars)=="TestUser";testEnv->ReleaseStringUTFChars(jname,chars);if(!good)return 37;
    testEnv->DeleteLocalRef(jname);testEnv->DeleteLocalRef(sc);testEnv->DeleteLocalRef(sess);
    if(!ApplyOfflineProfile(vm,env,minecraft,instance,world,"OtherUser",altStatus))return 39;
    testEnv->SetObjectField(object,world,savedWorld);if(RestoreOriginalSession(env,minecraft,instance,world,altStatus))return 40;
    testEnv->SetObjectField(object,world,nullptr);if(!RestoreOriginalSession(env,minecraft,instance,world,altStatus))return 41;
    sess=testEnv->GetObjectField(object,sid);sc=testEnv->GetObjectClass(sess);uname=testEnv->GetFieldID(sc,"username","Ljava/lang/String;");jname=static_cast<jstring>(testEnv->GetObjectField(sess,uname));chars=testEnv->GetStringUTFChars(jname,nullptr);good=std::string(chars)=="Original";testEnv->ReleaseStringUTFChars(jname,chars);if(!good)return 42;
    testEnv->DeleteLocalRef(jname);testEnv->DeleteLocalRef(sc);testEnv->DeleteLocalRef(sess);testEnv->SetObjectField(object,world,savedWorld);testEnv->DeleteLocalRef(savedWorld);
    RefreshJumpMapping();if(!jumpTicks||!jumpPlayerClass){std::cerr<<"Jump mapping was not found\n";return 17;}
    jobject testPlayer=testEnv->GetObjectField(object,player);if(!testPlayer)return 18;
    RefreshUseMapping();if(!itemInUseField||ReadItemUseState()!=ItemUseState::Idle){std::cerr<<"Item-use mapping was not found\n";return 27;}
    jclass itemClass=testEnv->FindClass("net/minecraft/item/ItemStack");if(!itemClass)return 28;
    jobject testItem=testEnv->NewObject(itemClass,testEnv->GetMethodID(itemClass,"<init>","()V"));if(!testItem)return 29;
    testEnv->SetObjectField(testPlayer,itemInUseField,testItem);
    if(ReadItemUseState()!=ItemUseState::Active){std::cerr<<"Active item use was not detected\n";return 30;}
    testEnv->SetObjectField(testPlayer,itemInUseField,nullptr);if(ReadItemUseState()!=ItemUseState::Idle)return 31;
    testEnv->SetIntField(testPlayer,jumpTicks,7);ResetJumpDelay();
    if(testEnv->GetIntField(testPlayer,jumpTicks)!=0){std::cerr<<"Jump delay was not reset\n";return 19;}
    testEnv->SetIntField(object,delay,7);ResetDelay();
    if(testEnv->GetIntField(object,delay)!=0){std::cerr<<"Delay was not reset\n";return 10;}
    jclass screenClass=testEnv->FindClass("net/minecraft/client/gui/GuiScreen");if(!screenClass)return 11;
    auto ctor=testEnv->GetMethodID(screenClass,"<init>","()V");if(!ctor)return 12;
    jobject openScreen=testEnv->NewObject(screenClass,ctor);if(!openScreen)return 13;
    testEnv->SetObjectField(object,screen,openScreen);testEnv->SetIntField(object,delay,7);ResetDelay();
    if(IsWorldReady()){std::cerr<<"Open screen was treated as gameplay\n";return 23;}
    testEnv->SetIntField(testPlayer,jumpTicks,7);ResetJumpDelay();
    if(testEnv->GetIntField(testPlayer,jumpTicks)!=7){std::cerr<<"Jump delay changed with a screen open\n";return 20;}
    if(testEnv->GetIntField(object,delay)!=7){std::cerr<<"Delay changed with a screen open\n";return 14;}
    testEnv->SetObjectField(object,screen,nullptr);testEnv->SetObjectField(object,world,nullptr);ResetDelay();
    if(IsWorldReady()){std::cerr<<"Missing world was treated as gameplay\n";return 24;}
    ResetJumpDelay();if(testEnv->GetIntField(testPlayer,jumpTicks)!=7){std::cerr<<"Jump delay changed without a world\n";return 21;}
    if(testEnv->GetIntField(object,delay)!=7){std::cerr<<"Delay changed without a world\n";return 15;}
    jclass obfuscated=testEnv->FindClass("ave");if(!obfuscated||!TryMapping(obfuscated,mappings[1])||!debugFpsField||testEnv->GetStaticIntField(obfuscated,debugFpsField)!=165){std::cerr<<"Obfuscated game FPS field was not mapped\n";return 26;}
    jobject omc=testEnv->GetStaticObjectField(obfuscated,instance);jobject ow=testEnv->GetObjectField(omc,world);testEnv->SetObjectField(omc,world,nullptr);
    if(!ApplyOfflineProfile(vm,env,minecraft,instance,world,"SecondUser",altStatus)){std::cerr<<"Obfuscated profile failed: "<<altStatus<<"\n";return 38;}
    testEnv->SetObjectField(omc,world,ow);testEnv->DeleteLocalRef(ow);testEnv->DeleteLocalRef(omc);
    RefreshUseMapping();if(!itemInUseField||ReadItemUseState()!=ItemUseState::Idle){std::cerr<<"Obfuscated item-use mapping failed\n";return 32;}
    jobject obfMc=testEnv->GetStaticObjectField(obfuscated,instance);jobject obfPlayer=testEnv->GetObjectField(obfMc,player);jclass obfItemClass=testEnv->FindClass("zx");
    jobject obfItem=testEnv->NewObject(obfItemClass,testEnv->GetMethodID(obfItemClass,"<init>","()V"));testEnv->SetObjectField(obfPlayer,itemInUseField,obfItem);
    if(ReadItemUseState()!=ItemUseState::Active){std::cerr<<"Obfuscated active item use was not detected\n";return 33;}
    ResetFields();testEnv->DeleteLocalRef(obfItem);testEnv->DeleteLocalRef(obfItemClass);testEnv->DeleteLocalRef(obfPlayer);testEnv->DeleteLocalRef(obfMc);testEnv->DeleteLocalRef(obfuscated);testEnv->DeleteLocalRef(testItem);testEnv->DeleteLocalRef(itemClass);testEnv->DeleteLocalRef(testPlayer);testEnv->DeleteLocalRef(openScreen);testEnv->DeleteLocalRef(screenClass);testEnv->DeleteLocalRef(object);testEnv->DeleteLocalRef(cls);
    if(testVm->DestroyJavaVM()!=JNI_OK)return 16;
    std::cout<<"PASS: mapping, in-world reset, open-screen and missing-world checks\n";return 0;
}



