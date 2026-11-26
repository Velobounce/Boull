#include <windows.h>
#include <jni.h>
#include <string>

JavaVM* vm{}; JNIEnv* env{}; jobject game{}; jfieldID counter{}; int zeros=0, ticks=0;
LRESULT CALLBACK WndProc(HWND window,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_TIMER){
        const int value=env->GetIntField(game,counter); if(value==0)++zeros;
        if(++ticks%8==0)env->SetIntField(game,counter,7);
        const auto title=L"Minecraft AutoClickV test fixture - zeros: "+std::to_wstring(zeros);
        SetWindowTextW(window,title.c_str()); return 0;
    }
    if(message==WM_DESTROY){KillTimer(window,1);PostQuitMessage(0);return 0;}
    return DefWindowProcW(window,message,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE,PWSTR,int){
    wchar_t home[32768]{};const DWORD n=GetEnvironmentVariableW(L"JAVA_HOME",home,32768);if(!n||n>=32768)return 1;
    const std::wstring jvmPath=std::wstring(home)+L"\\jre\\bin\\server\\jvm.dll";HMODULE lib=LoadLibraryW(jvmPath.c_str());if(!lib)return 2;
    using CreateVM=jint(JNICALL*)(JavaVM**,void**,void*);auto create=reinterpret_cast<CreateVM>(GetProcAddress(lib,"JNI_CreateJavaVM"));if(!create)return 3;
    char classes[32768]{};const DWORD count=GetEnvironmentVariableA("AUTOCLICKV_TEST_CLASSES",classes,32768);if(!count||count>=32768)return 4;
    std::string cp="-Djava.class.path="+std::string(classes);JavaVMOption option{};option.optionString=cp.data();JavaVMInitArgs args{};args.version=JNI_VERSION_1_8;args.nOptions=1;args.options=&option;
    if(create(&vm,reinterpret_cast<void**>(&env),&args)!=JNI_OK)return 5;
    jclass cls=env->FindClass("net/minecraft/client/Minecraft");if(!cls)return 6;
    jfieldID singleton=env->GetStaticFieldID(cls,"theMinecraft","Lnet/minecraft/client/Minecraft;");counter=env->GetFieldID(cls,"leftClickCounter","I");if(!singleton||!counter)return 7;
    game=env->NewGlobalRef(env->GetStaticObjectField(cls,singleton));if(!game)return 8;
    WNDCLASSW wc{};wc.hInstance=h;wc.lpfnWndProc=WndProc;wc.lpszClassName=L"LWJGL-AutoClickV-Test";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);if(!RegisterClassW(&wc))return 9;
    HWND window=CreateWindowW(wc.lpszClassName,L"Minecraft AutoClickV test fixture",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,460,180,nullptr,nullptr,h,nullptr);if(!window)return 10;
    if(!SetTimer(window,1,25,nullptr))return 11;ShowWindow(window,SW_SHOW);UpdateWindow(window);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}env->DeleteGlobalRef(game);vm->DestroyJavaVM();return 0;
}
