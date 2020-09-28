#include "il2cpp-appdata.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <thread>
#include "GameUtility.hpp"
#include "StringUtility.hpp"
#include "IterationCounter.hpp"
#include "D3D11Hooking.hpp"
#include "CWState.hpp"
#include "CWConstants.hpp"
#include "magic_enum.hpp"
#include "detours.h"
#include "helpers.h"
#include "radar.hpp"

using namespace app;

extern const LPCWSTR LOG_FILE = L"il2cpp-log.txt";

IDXGISwapChain* SwapChain;
ID3D11Device* Device;
ID3D11DeviceContext* Ctx;
ID3D11RenderTargetView* RenderTargetView;

D3D_PRESENT_FUNCTION OriginalD3DFunction;
WNDPROC OriginalWndProcFunction;
void (*OriginalHudFunction)(MethodInfo*);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ImGuiIO& io = ImGui::GetIO();
    POINT mPos;
    GetCursorPos(&mPos);
    ScreenToClient(CWState::Window, &mPos);

    ImGui::GetIO().MousePos.x = mPos.x;
    ImGui::GetIO().MousePos.y = mPos.y;

    if (uMsg == WM_KEYUP && wParam == VK_DELETE)
        CWState::ShowMenu = !CWState::ShowMenu;

    if (CWState::ShowMenu)
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

    return CallWindowProcW(OriginalWndProcFunction, hWnd, uMsg, wParam, lParam);
}

HRESULT __stdcall D3D_FUNCTION_HOOK(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
    if (!CWState::ImGuiInitialized)
    {
        SwapChain = pThis;
        pThis->GetDevice(__uuidof(ID3D11Device), (void**)&Device);
        Device->GetImmediateContext(&Ctx);

        DXGI_SWAP_CHAIN_DESC desc;

        pThis->GetDesc(&desc);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();

        CWState::Window = desc.OutputWindow;

        ImGui_ImplWin32_Init(CWState::Window);
        ImGui_ImplDX11_Init(Device, Ctx);

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        OriginalWndProcFunction = (WNDPROC)SetWindowLongW(CWState::Window, GWLP_WNDPROC, (LONG)WndProcHook);

        ImGui::GetIO().ImeWindowHandle = CWState::Window;

        ID3D11Texture2D* pBackBuffer;
        pThis->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        Device->CreateRenderTargetView(pBackBuffer, NULL, &RenderTargetView);
        pBackBuffer->Release();

        ImGui::GetStyle().WindowRounding = 0.0F;
        ImGui::GetStyle().ChildRounding = 0.0F;
        ImGui::GetStyle().FrameRounding = 0.0F;
        ImGui::GetStyle().GrabRounding = 0.0F;
        ImGui::GetStyle().PopupRounding = 0.0F;
        ImGui::GetStyle().ScrollbarRounding = 0.0F;

        CWState::ImGuiInitialized = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (CWState::ShowRadar) {
        Radar::RenderRadar(&CWState::ShowRadar, CWState::RadarZoom);
    }

    if (CWState::ShowMenu)
    {
        ImGuiStyle& st = ImGui::GetStyle();
        st.FrameBorderSize = 1.0f;
        st.FramePadding = ImVec2(4.0f, 2.0f);
        st.ItemSpacing = ImVec2(8.0f, 2.0f);
        st.WindowBorderSize = 1.0f;
        st.WindowRounding = 1.0f;
        st.ChildRounding = 1.0f;
        st.FrameRounding = 1.0f;
        st.ScrollbarRounding = 1.0f;
        st.GrabRounding = 1.0f;

        // Setup style
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.46f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
        colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
        colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
        colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.87f, 0.87f, 0.87f, 0.53f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);



        ImGuiWindowFlags window_flags = 1;
        static bool no_titlebar = false;
        if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize({ 970, 600 }, ImGuiCond_Always);
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);



        std::string kek = "Wertly Among Us Cheat (Simple Version)";
        ImGui::Begin((kek.c_str()), reinterpret_cast<bool*>(true), ImGuiWindowFlags_NoCollapse); // start open
        float LineWitdh = 970;
        ImVec2 Loaction = ImGui::GetCursorScreenPos();
        float DynamicRainbow = 0.01;
        static float staticHue = 0;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        staticHue -= DynamicRainbow;
        if (staticHue < -1.f) staticHue += 1.f;
        for (int i = 0; i < LineWitdh; i++)
        {
            float hue = staticHue + (1.f / (float)LineWitdh) * i;
            if (hue < 0.f) hue += 1.f;
            ImColor cRainbow = ImColor::HSV(hue, 1.f, 1.f);
            draw_list->AddRectFilled(ImVec2(Loaction.x + i, Loaction.y), ImVec2(Loaction.x + i + 1, Loaction.y + 4), cRainbow);
        }
        ImVec2 size = ImGui::GetItemRectSize();
       

        ImGui::Text("");
        ImGui::Text("");
        

        if (ImGui::CollapsingHeader("Game"))
        {
            
            ImGui::Text("");
            ImGui::Text("");
            if (ImGui::Button("Force Meeting"))
            {
                PlayerControl_CmdReportDeadBody((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, NULL, NULL);
            }
            ImGui::Checkbox("NoClip", &CWState::NoClip);
            ImGui::Checkbox("Mark Imposters", &CWState::MarkImposters);
            ImGui::Checkbox("Radar", &CWState::ShowRadar);
            ImGui::Text("Radar Zoom");
            ImGui::SliderFloat("##RadarZoom", &CWState::RadarZoom, 4.0F, 16.0F, "%.f", 1.0F);
        }
        ImGui::Spacing();
        ImGui::Text("");
        ImGui::Text("");
        if (ImGui::CollapsingHeader("Players") && IsInGame())
        {
            if (GetAllPlayers().size() > 0
                && CWState::MurderQueue.empty())
            {
                if (ImGui::Button("Murder Crewmates"))
                {
                    for (auto player : GetAllPlayers())
                    {
                        if (!GetPlayerData(player)->fields.IsImpostor)
                            CWState::MurderQueue.push(player);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Murder Imposters"))
                {
                    for (auto player : GetAllPlayers())
                    {
                        if (GetPlayerData(player)->fields.IsImpostor)
                            CWState::MurderQueue.push(player);
                    }
                }

                ImGui::Spacing();
            }

            for (auto player : GetAllPlayers())
            {
                auto data = GetPlayerData(player);
                auto name = GetUTF8StringFromNETString(data->fields.PlayerName);

                ImVec4 nameColor;

                if (data->fields.IsImpostor)
                    nameColor = AmongUsColorToImVec4((*Palette__TypeInfo)->static_fields->ImpostorRed);
                else
                    nameColor = AmongUsColorToImVec4((*Palette__TypeInfo)->static_fields->CrewmateBlue);

                if (data->fields.IsDead)
                    nameColor = AmongUsColorToImVec4((*Palette__TypeInfo)->static_fields->DisabledGrey);

                if (ImGui::Button((std::string("Kick") + std::string("##") + name).c_str()))
                {
                    CWState::KickTarget = player;
                }

                ImGui::SameLine();

                if (CWState::VoteTarget.has_value() && player == CWState::VoteTarget.value())
                {
                    if (ImGui::Button((std::string("Next Vote") + std::string("##") + name).c_str()))
                    {
                        CWState::VoteTarget = std::nullopt;
                    }
                }
                else
                {
                    if (ImGui::Button((std::string("Vote Off") + std::string("##") + name).c_str()))
                    {
                        CWState::VoteTarget = player;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button((std::string("Teleport") + std::string("##") + name).c_str()))
                {
                    Transform* localTransform = Component_get_transform((Component*)(*PlayerControl__TypeInfo)->static_fields->LocalPlayer, NULL);
                    Transform* playerTransform = Component_get_transform((Component*)player, NULL);
                    Transform_set_position(localTransform, Transform_get_position(playerTransform, NULL), NULL);
                }

                ImGui::SameLine();

                if (ImGui::Button((std::string("Murder") + std::string("##") + name).c_str()))
                    CWState::MurderTarget = player;

                ImGui::SameLine();

                ImGui::TextColored(nameColor, name.c_str());
            }
        }
        ImGui::Spacing();
        ImGui::Text("");
        ImGui::Text("");

        

        
        
        


        if (ImGui::CollapsingHeader("Chat"))
        {
            ImGui::Text("");
            ImGui::Text("");
            ImGui::Checkbox("All Players Spam", &CWState::AllPlayersSpam);
            ImGui::Checkbox("Spam Chat", &CWState::SpamChat);
            ImGui::Text("Message");
            ImGui::InputText("##SpamChatMessage", CWState::SpamMessage, IM_ARRAYSIZE(CWState::SpamMessage));
            ImGui::Text("Interval");
            CWState::ChatCounter.GenerateInput("##SpamChatInverval");
        }

       
        
 
        ImGui::Spacing();
        ImGui::Text("");
        ImGui::Text("");

        if (ImGui::CollapsingHeader("Misc"))
        {
            
             ImGui::Checkbox("Rainbow Shift", &CWState::ColorShift);

             ImGui::Spacing();

             for (int i = 0; i < (*Palette__TypeInfo)->static_fields->PlayerColors->max_length; i++)
             {
                 ImGui::PushStyleColor(ImGuiCol_Button, AmongUsColorToImVec4(GetPlayerColor(i)));
                 if (ImGui::Button((std::string("Color ") + std::to_string(i)).c_str()))
                     CWState::ColorTarget = i;
                 ImGui::PopStyleColor();

                 if ((*PlayerControl__TypeInfo)->static_fields->LocalPlayer != NULL
                     && GetPlayerData((*PlayerControl__TypeInfo)->static_fields->LocalPlayer)->fields.ColorId == i)
                 {
                     ImGui::SameLine();
                     ImGui::Text("Selected");
                 }
             }
             ImGui::Text("Kill Cooldown Modifier");
             ImGui::Checkbox("##KillCooldownModifierCheckbox", &CWState::ModifyKillCooldown);
             ImGui::SameLine();
             ImGui::SliderFloat("##KillCooldown", &CWState::KillCooldownModifier, 0.001F, 60);

             ImGui::Text("Kill Distance Modifier");
             ImGui::Checkbox("##KillDistanceModifierCheckbox", &CWState::ModifyKillDistance);

             String__Array* killDistancesNames = (*GameOptionsData__TypeInfo)->static_fields->KillDistanceStrings;

             // yes it is hardcoded, but the array bounds are null
             // maybe it's some kind of optimization on readonly fields?
             for (int i = 0; i < CWConstants::KILL_DISTANCES_LENGTH; i++)
             {
                 ImGui::SameLine();
                 ImGui::RadioButton(GetUTF8StringFromNETString(killDistancesNames->vector[i]).c_str(), &CWState::KillDistanceModifier, i);
             }
            
        }
        ImGui::Spacing();
        ImGui::Text("");
        ImGui::Text("");
        if (ImGui::CollapsingHeader("Info")) {

        ImGui::Text("");
        ImGui::Text("");
            ImGui::Text("Menu Re-Touched By: jury#9999");
            if (ImGui::Button("Instagram")) {
                system("start https://instagram.com/benjajury_");

            }
            ImGui::Text("Enjoy.");
        }

        ImGui::End();
    }
    
    ImGui::EndFrame();
    ImGui::Render();

    Ctx->OMSetRenderTargets(1, &RenderTargetView, NULL);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return OriginalD3DFunction(pThis, SyncInterval, Flags);
}

void HudHook(MethodInfo* m)
{
    OriginalHudFunction(m);

    if (CWState::VoteTarget.has_value()
        && (*MeetingHud__TypeInfo)->static_fields->Instance != NULL)
    {       
        // Class appears to be 4 bytes off?
        // here we are checking vote state which appears to be in VoteEndingSound
        MeetingHud_VoteStates__Enum state =
            (MeetingHud_VoteStates__Enum)((intptr_t)(*MeetingHud__TypeInfo)->static_fields->Instance->fields.VoteEndingSound);

        if (state == MeetingHud_VoteStates__Enum_NotVoted
            || state == MeetingHud_VoteStates__Enum_Voted)
        {
            for (auto player : GetAllPlayers())
            {
                MeetingHud_CmdCastVote((*MeetingHud__TypeInfo)->static_fields->Instance,
                    GetPlayerData(player)->fields.PlayerId,
                    GetPlayerData(CWState::VoteTarget.value())->fields.PlayerId,
                    NULL);
            }

            CWState::VoteTarget = std::nullopt;
        }
    }

    if (CWState::AllClothesCounter.ProcessAction() && CWState::ShiftAllClothes)
    {
        for (auto player : GetAllPlayers())
        {
            // would write way to find max bound for these two
            // but im too lazy 
            PlayerControl_RpcSetHat(player, randi(0, 10), NULL);
            PlayerControl_RpcSetSkin(player, randi(0, 10), NULL);
        }
    }

    if (CWState::ChatCounter.ProcessAction() && CWState::SpamChat)
    {
        if (CWState::AllPlayersSpam)
        {
            for (auto player : GetAllPlayers())
                PlayerControl_RpcSendChat(player, CreateNETStringFromANSI(CWState::SpamMessage), NULL);
        }
        else
        {
            PlayerControl_RpcSendChat((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, CreateNETStringFromANSI(CWState::SpamMessage), NULL);
        }
    };

    if (CWState::ColorCounter.ProcessAction() && CWState::ColorShift)
    {
        CWState::CurrentColor = GetNextColor(CWState::CurrentColor);

        while (!CheckColorAvailable(CWState::CurrentColor))
            CWState::CurrentColor = GetNextColor(CWState::CurrentColor);

        PlayerControl_CmdCheckColor((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, CWState::CurrentColor, NULL);
    }

    if (CWState::AllColorsCounter.ProcessAction() && CWState::ShiftAllColors)
    {
        for (auto player : GetAllPlayers())
        {
            auto data = GetPlayerData(player);
            auto playerColorId = data->fields.ColorId;
            playerColorId = GetNextColor(playerColorId);

            while (!CheckColorAvailable(playerColorId))
                playerColorId = GetNextColor(playerColorId);

            PlayerControl_CmdCheckColor(player, playerColorId, NULL);
        }
    }

    if (CWState::KickTarget.has_value())
    {
        VoteBanSystem* vbSystem = (*VoteBanSystem__TypeInfo)->static_fields->Instance;
        InnerNetClient* net = (InnerNetClient*)(*AmongUsClient__TypeInfo)->static_fields->Instance;
        int32_t victimId = InnerNetClient_GetClientIdFromCharacter(net, (InnerNetObject*)CWState::KickTarget.value(), NULL);
        int32_t oldClientId = net->fields.ClientId;

        for (auto player : GetAllPlayers())
        {
            net->fields.ClientId = InnerNetClient_GetClientIdFromCharacter(net, (InnerNetObject*)player, NULL);
            VoteBanSystem_CmdAddVote((*VoteBanSystem__TypeInfo)->static_fields->Instance, victimId, NULL);
        }

        net->fields.ClientId = oldClientId;

        CWState::KickTarget = std::nullopt;
    }

    if (CWState::PinDoors)
    {
        for (auto entry : CWState::PinnedDoors)
        {
            ShipStatus_RpcCloseDoorsOfType((*ShipStatus__TypeInfo)->static_fields->Instance, entry, NULL);
        }
    }

    if (CWState::MurderTarget.has_value())
    {
        if (GetPlayerData((*PlayerControl__TypeInfo)->static_fields->LocalPlayer)->fields.IsImpostor)
        {
            PlayerControl_RpcMurderPlayer((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, CWState::MurderTarget.value(), NULL);
        }
        else
        {
            for (auto player : GetAllPlayers())
            {
                if (GetPlayerData(player)->fields.IsImpostor && !GetPlayerData(player)->fields.IsDead)
                {
                    PlayerControl_RpcMurderPlayer(player, CWState::MurderTarget.value(), NULL);
                    break;
                }
            }
        }
        
        CWState::MurderTarget = std::nullopt;
    }

    if (CWState::ColorTarget.has_value())
    {
        PlayerControl_CmdCheckColor((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, CWState::ColorTarget.value(), NULL);
        CWState::ColorTarget = std::nullopt;
    }

    while (!CWState::MurderQueue.empty())
    {
        auto front = CWState::MurderQueue.front();
        if (GetPlayerData((*PlayerControl__TypeInfo)->static_fields->LocalPlayer)->fields.IsImpostor)
        {
            PlayerControl_RpcMurderPlayer((*PlayerControl__TypeInfo)->static_fields->LocalPlayer, front, NULL);
        }
        else
        {
            for (auto player : GetAllPlayers())
            {
                if (GetPlayerData(player)->fields.IsImpostor && !GetPlayerData(player)->fields.IsDead)
                {
                    PlayerControl_RpcMurderPlayer(player, front, NULL);
                    break;
                }
            }
        }
        CWState::MurderQueue.pop();
    }
    
    if (CWState::ModifySpeed)
        (*PlayerControl__TypeInfo)->static_fields->GameOptions->fields.PlayerSpeedMod = CWState::SpeedModifier;

    if (CWState::ModifyKillCooldown)
        (*PlayerControl__TypeInfo)->static_fields->GameOptions->fields.KillCooldown = CWState::KillCooldownModifier;

    if (CWState::ModifyKillDistance)
        (*PlayerControl__TypeInfo)->static_fields->GameOptions->fields.KillDistance = CWState::KillDistanceModifier;

    if (CWState::ModifyLight)
    {
        (*PlayerControl__TypeInfo)->static_fields->GameOptions->fields.ImpostorLightMod = CWState::LightModifier;
        (*PlayerControl__TypeInfo)->static_fields->GameOptions->fields.CrewLightMod = CWState::LightModifier;
    }

    if (CWState::NoClip)
    {
        auto comp = Component_get_gameObject((Component*)(*PlayerControl__TypeInfo)->static_fields->LocalPlayer, NULL);
        GameObject_set_layer(comp, LayerMask_NameToLayer(CreateNETStringFromANSI("Ghost"), NULL), NULL);
    }

    if (CWState::MarkImposters)
        for (auto player : GetAllPlayers())
        {
            auto data = GetPlayerData(player);

            // Interestingly enough, RemainingEmergencies is actually a pointer to the PlayerControl field nameText...
            TextRenderer* nameText = (TextRenderer*)(player->fields.RemainingEmergencies);

            nameText->fields.Color = data->fields.IsImpostor
                ? (*Palette__TypeInfo)->static_fields->ImpostorRed
                : (*Palette__TypeInfo)->static_fields->CrewmateBlue;
        }
    
    //auto hudManager = (HudManager*)DestroyableSingleton_1_InnerNet_InnerNetServer__get_Instance(*(MethodInfo**)(GetBaseAddress() + 0x00E2577C));
}

bool HookFunction(PVOID* original, PVOID detour, const char* funcName)
{
    LONG err = DetourAttach(original, detour);
    if (err != 0)
    {
        std::cout << "Failed to hook a function: " << funcName << "\n";
        return false;
    }
    else
    {
        std::cout << "Successfully hooked a function: " << funcName << "\n";
        return true;
    }
}

void Run()
{
    NewConsole();

    std::cout << "Initializing...\n";

    OriginalHudFunction = KeyboardJoystick_HandleHud;
    OriginalD3DFunction = GetD3D11PresentFunction();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (!HookFunction(&(PVOID&)OriginalHudFunction, HudHook, "KeyboardJoystick_HandleHud"))
        return;

    if (OriginalD3DFunction == NULL || !HookFunction(&(PVOID&)OriginalD3DFunction, D3D_FUNCTION_HOOK, "D3D11Present"))
        return;

    DetourTransactionCommit();

    std::cout << "Initialization Complete\n";

    system("cls");

    std::cout << CWConstants::CHRISTWARE_ASCII << "\n\n\t\tAmong Us Edition";
}