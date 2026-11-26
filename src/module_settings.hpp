#pragma once
struct ModuleSettings {
    int leftMin=10, leftMax=14, rightMin=1, rightMax=1;
    bool inventory=true, breakBlocks=true;
    bool rightGuardAvailable=false;
    bool arrayList=false, notifications=true, watermark=false;
    bool arrayTags=true, arrayModern=true, arrayShadow=true;
    bool hideLeftFromArray=false, hideRightFromArray=false;
    bool hideHitDelayFromArray=false, hideJumpDelayFromArray=false;
    int hudTextScale=130, arrayPositionY=1, arrayOpacity=40;
    bool watermarkBackground=true, watermarkShadow=true, watermarkFps=true;
    int watermarkX=50, watermarkY=0, watermarkOpacity=40;
    char watermarkName[64]="Boull";
};
inline ModuleSettings moduleSettings;
