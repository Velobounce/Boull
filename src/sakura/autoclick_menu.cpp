#include "ui.hpp"
#include <string>
#include "../module_settings.hpp"
#include "../offline_profiles.hpp"

namespace AnimeUI {
void RenderAutoClickMenu(bool& left, bool& right, bool& delay, bool& jump, bool mapped, bool jumpMapped, bool hudReady, int& cps,
                         bool& capture, const std::string& bind, const std::string& status,
                         bool& requestBind, bool& requestUnload) {
    ImGuiIO& io=ImGui::GetIO();
    g_dt=ImMin(io.DeltaTime,0.05f); g_time+=g_dt; g_s=1.0f;
    g_accent=g_rainbow ? Hsv(std::fmod(g_time*0.10f,1.0f),0.70f,1.0f) : ImGui::ColorConvertFloat4ToU32(g_accent_f);
    if(g_custom_background){
        const ImU32 bg=ImGui::ColorConvertFloat4ToU32(g_background_f);
        const float luminance=.2126f*g_background_f.x+.7152f*g_background_f.y+.0722f*g_background_f.z;
        const bool dark=luminance<.50f;
        const ImU32 white=IM_COL32(255,255,255,255), black=IM_COL32(15,17,24,255);
        Pal::Bg0=bg; Pal::Bg1=Mix(bg,dark?white:black,.07f);
        Pal::Head0=Mix(bg,g_accent,.35f); Pal::Head1=Mix(bg,g_accent,.20f);
        Pal::Card=Mix(bg,white,dark?.10f:.50f);
        Pal::CardSoft=Mix(bg,white,dark?.08f:.35f);
        Pal::Side=Mix(bg,white,dark?.08f:.25f);
        Pal::Line=Mix(bg,dark?white:black,dark?.22f:.20f);
        Pal::LineSoft=Mix(bg,dark?white:black,dark?.15f:.12f);
        Pal::Pink=g_accent; Pal::PinkD=Mix(g_accent,dark?white:black,.20f);
        Pal::Lilac=g_accent; Pal::Sky=g_accent; Pal::Mint=g_accent;
        Pal::Cream=dark?white:IM_COL32(255,248,231,255);
        Pal::Text=dark?IM_COL32(246,246,250,255):IM_COL32(45,45,55,255);
        Pal::TextDim=dark?IM_COL32(190,190,200,255):IM_COL32(110,110,120,255);
        Pal::Track=Mix(bg,dark?white:black,dark?.20f:.12f);
        Pal::FrameBg=Mix(bg,white,dark?.14f:.45f);
        Pal::FrameHov=Mix(bg,g_accent,.23f);Pal::FrameAct=Mix(bg,g_accent,.35f);
        Pal::Shadow=dark?black:Mix(bg,black,.40f);
    }
    ApplyTheme();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(820,540));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##autoclickv_sakura",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    const ImVec2 p=ImGui::GetWindowPos(), sz=ImGui::GetWindowSize();
    const ImRect win(p,p+sz);
    ImFont* font=ImGui::GetFont();
    ImFont* fontb=g_font_bold ? g_font_bold : font;
    dl->AddRectFilled(p,p+sz,Mix(Pal::Bg0,Pal::Bg1,0.45f),18.0f);


    dl->AddRectFilled(p,p+ImVec2(sz.x,58),Mix(Pal::Head0,Pal::Head1,0.40f),18,ImDrawFlags_RoundCornersTop);
    dl->AddLine(p+ImVec2(14,57),p+ImVec2(sz.x-14,57),Alpha(Pal::White,0.75f),1.5f);
    dl->AddRect(p,p+sz,Alpha(Pal::Line,0.9f),18,0,1.2f);
    if(g_logoTexture!=ImTextureID_Invalid)dl->AddImage(ImTextureRef(g_logoTexture),p+ImVec2(20,11),p+ImVec2(186,47));
    const ImRect side(p+ImVec2(14,68),p+ImVec2(214,526));
    dl->AddRectFilled(side.Min+ImVec2(0,4),side.Max+ImVec2(0,4),Alpha(Pal::Shadow,0.22f*g_glow),18);
    dl->AddRectFilled(side.Min,side.Max,Alpha(Pal::Side,g_card_a),18);
    dl->AddRect(side.Min,side.Max,Alpha(Pal::Line,0.95f),18,0,1.2f);
    const ImRect card(p+ImVec2(226,68),p+ImVec2(806,526));
    dl->AddRectFilled(card.Min+ImVec2(0,4),card.Max+ImVec2(0,4),Alpha(Pal::Shadow,0.26f*g_glow),20);
    dl->AddRectFilled(card.Min,card.Max,Alpha(Pal::Card,g_card_a),20);
    dl->AddRect(card.Min,card.Max,Alpha(Pal::LineSoft,0.95f),20,0,1.2f);
    static int tab=0;
    static int previousTab=0,displayedModule=0;
    static float optionsProgress=0.0f;
    int& selectedModule=g_selectedModule;
    (void)cps;
    PushAccentStyle();
    ImGui::SetCursorPos(ImVec2(24,78)); if(TabButton("Modules",tab==0,ImVec2(180,40),0)){tab=0;selectedModule=0;}
    ImGui::SetCursorPos(ImVec2(24,126)); if(TabButton("Visuals",tab==2,ImVec2(180,40),2)){tab=2;selectedModule=0;}
    ImGui::SetCursorPos(ImVec2(24,174)); if(TabButton("Alt Manager",tab==3,ImVec2(180,40),3)){tab=3;selectedModule=0;}
    ImGui::SetCursorPos(ImVec2(24,222)); if(TabButton("Appearance",tab==1,ImVec2(180,40),1)){tab=1;selectedModule=0;}
    ImGui::SetCursorPos(ImVec2(28,465));
    static bool showUnload=false; if(ImGui::InvisibleButton("##settings_gear",ImVec2(34,34)))showUnload=!showUnload;
    const ImVec2 gear=p+ImVec2(45,482);const ImU32 gearColor=ImGui::IsItemHovered()?g_accent:Pal::TextDim;
    dl->AddCircle(gear,8.0f,gearColor,16,3.0f);
    for(int i=0;i<8;++i){const float a=static_cast<float>(i)*kPi/4.0f;const ImVec2 v(std::cos(a),std::sin(a));dl->AddLine(gear+v*9.0f,gear+v*13.0f,gearColor,3.0f);}
    dl->AddCircleFilled(gear,2.5f,gearColor);
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Settings");
    if(showUnload){
        dl->AddRectFilled(p+ImVec2(24,406),p+ImVec2(162,455),Pal::Side,12.0f);
        dl->AddRect(p+ImVec2(24,406),p+ImVec2(162,455),Pal::Line,12.0f);
        dl->AddTriangleFilled(p+ImVec2(38,454),p+ImVec2(52,454),p+ImVec2(45,463),Pal::Side);
        ImGui::SetCursorPos(ImVec2(33,415));if(PillButton("Unload",ImVec2(120,30),true)){requestUnload=true;showUnload=false;}
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&!ImGui::IsMouseHoveringRect(p+ImVec2(24,406),p+ImVec2(162,455))&&!ImGui::IsMouseHoveringRect(p+ImVec2(28,465),p+ImVec2(62,499)))showUnload=false;
    }
    if(tab!=previousTab){optionsProgress=0.0f;displayedModule=0;previousTab=tab;}
    if(selectedModule!=0)displayedModule=selectedModule;
    optionsProgress=ExpLerp(optionsProgress,selectedModule!=0?1.0f:0.0f,14.0f,g_dt);
    if(optionsProgress<0.001f&&selectedModule==0){optionsProgress=0.0f;displayedModule=0;}
    const float moduleX=246.0f;
    dl->AddText(fontb,17,p+ImVec2(246,86),Pal::Text,tab==0?"MODULES":(tab==1?"APPEARANCE":(tab==3?"ALT MANAGER":"VISUALS")));
    dl->AddRectFilledMultiColor(p+ImVec2(246,124),p+ImVec2(316,126.5f),Alpha(g_accent,0),Alpha(g_accent,.9f),Alpha(g_accent,0),Alpha(g_accent,0));
    const auto moduleRow=[&](const char* name,bool& enabled,int index,int id,bool available=true){
        const float vertical=(std::min)(1.0f,optionsProgress*2.0f);
        const float horizontal=(std::max)(0.0f,optionsProgress*2.0f-1.0f);
        const float x=Lerpf(246.0f+(index%2)*280.0f,246.0f,horizontal);
        const float y=Lerpf(164.0f+(index/2)*72.0f,164.0f+index*42.0f,vertical);
        const float height=Lerpf(60.0f,36.0f,vertical);
        ImGui::SetCursorPos(ImVec2(x,y));if(!available)ImGui::BeginDisabled();ToggleRow(name,&enabled,260,height);
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)&&ImGui::IsMouseClicked(ImGuiMouseButton_Right))selectedModule=selectedModule==id?0:id;
        if(!available)ImGui::EndDisabled();
    };
    const auto sliderInt=[&](const char* name,int& value,float min,float max,float y){float v=static_cast<float>(value);ImGui::SetCursorPos(ImVec2(526,y));if(SliderRow(name,&v,min,max,"%.0f",260))value=static_cast<int>(v+.5f);};
    if(tab==0){
        SectionTag(dl,"BOULL",p+ImVec2(moduleX,136));
        moduleRow("AutoClicker",left,0,1);moduleRow("RightClicker",right,1,2,moduleSettings.rightGuardAvailable);
        moduleRow("HitDelayFix",delay,2,3,mapped);moduleRow("NoJumpDelay",jump,3,4,jumpMapped);
        if(selectedModule!=0)displayedModule=selectedModule;
        const int optionVertices=dl->VtxBuffer.Size;
        const bool optionsDisabled=selectedModule==0||optionsProgress<0.90f;
        if(optionsDisabled)ImGui::BeginDisabled();
        if(displayedModule==1||displayedModule==2){
            SectionTag(dl,displayedModule==1?"AUTOCLICKER":"RIGHTCLICKER",p+ImVec2(526,136));
            int& min=displayedModule==1?moduleSettings.leftMin:moduleSettings.rightMin;
            int& max=displayedModule==1?moduleSettings.leftMax:moduleSettings.rightMax;
            sliderInt("Min CPS",min,1,50,164);if(min>max)min=max;
            sliderInt("Max CPS",max,1,50,216);if(max<min)max=min;
            if(displayedModule==1){
                ImGui::SetCursorPos(ImVec2(526,274));ToggleRow("Disable in inventory",&moduleSettings.inventory,260);
                ImGui::SetCursorPos(ImVec2(526,316));ToggleRow("Break blocks",&moduleSettings.breakBlocks,260);
                ImGui::SetCursorPos(ImVec2(526,358));ToggleRow("Only while holding",&moduleSettings.onlyWhileHolding,260);
                ImGui::SetCursorPos(ImVec2(526,400));const std::string label=capture?"Press a key or mouse button...":"Bind: "+bind;
                if(PillButton(label.c_str(),ImVec2(250,38),true))requestBind=true;
                dl->AddText(font,13,p+ImVec2(526,444),Pal::TextDim,status.c_str());
                ImGui::SetCursorPos(ImVec2(526,468));ToggleRow("Hide from ArrayList",&moduleSettings.hideLeftFromArray,260);
            }else{
                ImGui::SetCursorPos(ImVec2(526,274));ToggleRow("Hide from ArrayList",&moduleSettings.hideRightFromArray,260);
                dl->AddText(font,13,p+ImVec2(526,326),Pal::TextDim,"Item protection: up to about 14 CPS.");
            }
        }else if(displayedModule==3||displayedModule==4){
            SectionTag(dl,"OPTIONS",p+ImVec2(526,136));
            ImGui::SetCursorPos(ImVec2(526,164));
            bool& hidden=displayedModule==3?moduleSettings.hideHitDelayFromArray:moduleSettings.hideJumpDelayFromArray;
            ToggleRow("Hide from ArrayList",&hidden,260);
        }
        if(optionsDisabled)ImGui::EndDisabled();
        for(int i=optionVertices;i<dl->VtxBuffer.Size;i++){ImU32& color=dl->VtxBuffer[i].col;const auto alpha=static_cast<ImU32>(((color>>IM_COL32_A_SHIFT)&0xff)*optionsProgress);color=(color&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);}
        if(!moduleSettings.rightGuardAvailable)dl->AddText(font,13,p+ImVec2(moduleX,433),Pal::TextDim,"RightClicker: waiting for a compatible player.");
        if(!mapped)dl->AddText(font,13,p+ImVec2(moduleX,455),Pal::TextDim,"HitDelayFix unavailable: fields not recognized.");
        if(!jumpMapped)dl->AddText(font,13,p+ImVec2(moduleX,477),Pal::TextDim,"NoJumpDelay unavailable: waiting for a compatible player.");
    } else if(tab==1) {
        static bool editBackground=false;
        SectionTag(dl,"EDIT COLOR",p+ImVec2(246,136));
        ImGui::SetCursorPos(ImVec2(246,164));
        if(PillButton("Accent",ImVec2(115,30),editBackground))editBackground=false;
        ImGui::SetCursorPos(ImVec2(371,164));
        if(PillButton("Background",ImVec2(115,30),!editBackground))editBackground=true;
        const ImU32 colors[]={IM_COL32(255,138,192,255),IM_COL32(186,156,255,255),IM_COL32(141,227,255,255),IM_COL32(168,240,208,255),IM_COL32(255,95,95,255),IM_COL32(255,156,73,255),IM_COL32(255,220,90,255),IM_COL32(90,205,125,255),IM_COL32(52,186,192,255),IM_COL32(90,130,255,255),IM_COL32(34,38,52,255),IM_COL32(235,235,240,255)};
        for(int i=0;i<12;i++){
            ImGui::SetCursorPos(ImVec2(246+(i%6)*40.0f,214+(i/6)*40.0f));
            char id[20];std::snprintf(id,sizeof(id),"color%d",i);
            if(SwatchButton(id,colors[i],ImVec2(30,30))){if(editBackground){g_background_f=U4(colors[i]);g_custom_background=true;}else{g_accent_f=U4(colors[i]);g_rainbow=false;}Burst(p+ImVec2(261+(i%6)*40.0f,229+(i/6)*40.0f),12,colors[i],200.0f);}
        }
        SectionTag(dl,"CUSTOM COLOR",p+ImVec2(526,136));
        ImGui::SetCursorPos(ImVec2(526,164)); ImGui::SetNextItemWidth(200);
        ImVec4& selected=editBackground?g_background_f:g_accent_f;
        if(ImGui::ColorPicker4("##custom_color",&selected.x,ImGuiColorEditFlags_NoAlpha|ImGuiColorEditFlags_NoSidePreview|ImGuiColorEditFlags_NoSmallPreview|ImGuiColorEditFlags_NoInputs)){if(editBackground)g_custom_background=true;else g_rainbow=false;}
    } else if(tab==3) {
        OfflineProfiles::Load();
        SectionTag(dl,"OFFLINE PROFILES",p+ImVec2(246,136));
        ImGui::SetCursorPos(ImVec2(246,164));ImGui::SetNextItemWidth(250);
        ImGui::InputTextWithHint("##alt_name","Nickname (3-16 characters)",OfflineProfiles::input,sizeof(OfflineProfiles::input),ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SetCursorPos(ImVec2(510,164));
        if(PillButton("Add and use",ImVec2(180,30),true)){std::string name=OfflineProfiles::input;if(OfflineProfiles::Add(name)){OfflineProfiles::requested=name;OfflineProfiles::input[0]=0;}}
        ImGui::SetCursorPos(ImVec2(246,205));ImGui::TextWrapped("%s",OfflineProfiles::status.c_str());
        ImGui::SetCursorPos(ImVec2(246,235));ImGui::Text("Current profile: %s",OfflineProfiles::current.empty()?"None":OfflineProfiles::current.c_str());
        ImGui::SetCursorPos(ImVec2(510,228));if(PillButton("Restore original",ImVec2(180,30),true))OfflineProfiles::restoreRequested=true;
        ImGui::SetCursorPos(ImVec2(246,275));ImGui::BeginChild("##alt_list",ImVec2(530,220),false);
        for(size_t i=0;i<OfflineProfiles::profiles.size();){
            ImGui::PushID(static_cast<int>(i));ImGui::TextUnformatted(OfflineProfiles::profiles[i].c_str());ImGui::SameLine(250);
            if(ImGui::Button("Use"))OfflineProfiles::requested=OfflineProfiles::profiles[i];ImGui::SameLine(330);
            const bool remove=ImGui::Button("Remove");ImGui::PopID();if(remove)OfflineProfiles::Remove(i);else ++i;
        }
        ImGui::EndChild();
    } else {
        SectionTag(dl,"VISUALS",p+ImVec2(moduleX,136));
        moduleRow("ArrayList",moduleSettings.arrayList,0,5,hudReady);
        moduleRow("Notifications",moduleSettings.notifications,1,6,hudReady);
        moduleRow("Watermark",moduleSettings.watermark,2,7,hudReady);
        if(!hudReady)dl->AddText(font,14,p+ImVec2(moduleX,300),Pal::TextDim,"HUD is unavailable on this computer.");
        if(selectedModule!=0)displayedModule=selectedModule;
        const int optionVertices=dl->VtxBuffer.Size;
        const bool optionsDisabled=selectedModule==0||optionsProgress<0.90f;
        if(optionsDisabled)ImGui::BeginDisabled();
        if(displayedModule==5){
            SectionTag(dl,"ARRAYLIST",p+ImVec2(526,136));
            ImGui::SetCursorPos(ImVec2(526,164));ToggleRow("Information",&moduleSettings.arrayTags,260);
            ImGui::SetCursorPos(ImVec2(526,206));ToggleRow("Modern style",&moduleSettings.arrayModern,260);
            ImGui::SetCursorPos(ImVec2(526,248));ToggleRow("Text shadow",&moduleSettings.arrayShadow,260);
            sliderInt("HUD scale (%)",moduleSettings.hudTextScale,70,180,296);
            sliderInt("Position Y (%)",moduleSettings.arrayPositionY,0,100,348);
            if(moduleSettings.arrayModern)sliderInt("Background opacity (%)",moduleSettings.arrayOpacity,0,100,400);
        }else if(displayedModule==7){
            SectionTag(dl,"WATERMARK",p+ImVec2(526,136));
            ImGui::SetCursorPos(ImVec2(526,162));ImGui::SetNextItemWidth(260);ImGui::InputText("##watermark_name",moduleSettings.watermarkName,sizeof(moduleSettings.watermarkName));
            ImGui::SetCursorPos(ImVec2(526,208));ToggleRow("Show FPS",&moduleSettings.watermarkFps,260);
            ImGui::SetCursorPos(ImVec2(526,250));ToggleRow("Background",&moduleSettings.watermarkBackground,260);
            ImGui::SetCursorPos(ImVec2(526,292));ToggleRow("Text shadow",&moduleSettings.watermarkShadow,260);
            sliderInt("HUD scale (%)",moduleSettings.hudTextScale,70,180,330);
            sliderInt("Position X (%)",moduleSettings.watermarkX,0,100,374);
            sliderInt("Position Y (%)",moduleSettings.watermarkY,0,100,418);
            if(moduleSettings.watermarkBackground)sliderInt("Background opacity (%)",moduleSettings.watermarkOpacity,0,100,462);
        }else if(displayedModule==6){
            SectionTag(dl,"NOTIFICATIONS",p+ImVec2(526,136));dl->AddText(font,14,p+ImVec2(526,170),Pal::TextDim,"This module has no options.");
        }
        if(optionsDisabled)ImGui::EndDisabled();
        for(int i=optionVertices;i<dl->VtxBuffer.Size;i++){ImU32& color=dl->VtxBuffer[i].col;const auto alpha=static_cast<ImU32>(((color>>IM_COL32_A_SHIFT)&0xff)*optionsProgress);color=(color&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);}
    }
    PopAccentStyle();
    UpdateAndDrawSparks(dl);
    ImGui::End();
    Apply3DTilt(dl,win);
}
}








