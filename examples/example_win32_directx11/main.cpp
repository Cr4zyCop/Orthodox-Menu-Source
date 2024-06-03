// Dear ImGui: standalone example application for DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "fonts.h"
#include "imgui_internal.h"
#include "settings.h"
#include "images.h"
#include <D3DX11tex.h>
#include "imgui_combo.hpp"
#pragma comment(lib, "D3DX11.lib")
// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <chrono>
#include <algorithm>
using namespace ImGui;
struct Notification {
    int id;
    std::string message;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
};

class NotificationSystem {
public:
    NotificationSystem() : notificationIdCounter(0) {}

    void AddNotification(const std::string& message, int durationMs) {
        auto now = std::chrono::steady_clock::now();
        auto endTime = now + std::chrono::milliseconds(durationMs);
        notifications.push_back({ notificationIdCounter++, message, now, endTime });
    }

    void DrawNotifications() {
        auto now = std::chrono::steady_clock::now();

        std::sort(notifications.begin(), notifications.end(),
            [now](const Notification& a, const Notification& b) -> bool {
                float durationA = std::chrono::duration_cast<std::chrono::milliseconds>(a.endTime - a.startTime).count();
                float elapsedA = std::chrono::duration_cast<std::chrono::milliseconds>(now - a.startTime).count();
                float percentageA = (durationA - elapsedA) / durationA;

                float durationB = std::chrono::duration_cast<std::chrono::milliseconds>(b.endTime - b.startTime).count();
                float elapsedB = std::chrono::duration_cast<std::chrono::milliseconds>(now - b.startTime).count();
                float percentageB = (durationB - elapsedB) / durationB;

                return percentageA < percentageB;
            }
        );

        int currentNotificationPosition = 0;

        for (auto& notification : notifications) {
            if (now < notification.endTime) {
                float duration = std::chrono::duration_cast<std::chrono::milliseconds>(notification.endTime - notification.startTime).count();
                float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - notification.startTime).count();
                float percentage = (duration - elapsed) / duration * 100.0f;

                ShowNotification(currentNotificationPosition, notification.message, percentage);
                currentNotificationPosition++;
            }
        }

        notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
            [now](const Notification& notification) { return now >= notification.endTime; }),
            notifications.end());
    }
   
private:
    std::vector<Notification> notifications;
    int notificationIdCounter;

    void ShowNotification(int position, const std::string& message, float percentage) {

        float duePercentage = 100.0f - percentage;
        float alpha = percentage > 10.0f ? 1.0f : percentage / 10.0f;
        ImGui::GetStyle().WindowPadding = ImVec2(15, 10);

        ImGui::SetNextWindowPos(ImVec2(duePercentage < 15.f ? duePercentage : 15.f, 15 + position * 90));

        ImGui::Begin(("##NOTIFY" + std::to_string(position)).c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImVec2 pos = ImGui::GetWindowPos(), spacing = ImGui::GetStyle().ItemSpacing, region = ImGui::GetContentRegionMax();


        GetBackgroundDrawList()->AddRectFilledMultiColor(pos, pos + region, ImGui::GetColorU32(colors::accent_color, alpha), ImGui::GetColorU32(colors::accent_color, 0.01f), ImGui::GetColorU32(colors::accent_color, 0.01f), ImGui::GetColorU32(colors::accent_color, alpha));
        GetBackgroundDrawList()->AddRectFilled(pos, pos + region, ImGui::GetColorU32(colors::menu::window_bg, 0.4f),5);
        GetBackgroundDrawList()->AddRect(pos, pos + region, ImGui::GetColorU32(colors::menu::watermark_border, alpha), 5);

       
            GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, region.y - 3), pos + ImVec2(region.x * (duePercentage / 100.0f), region.y), ImGui::GetColorU32(colors::accent_color, alpha), 5);

        PushFont(fonts::inter_bold_font);
        ImGui::TextColored(ImColor(GetColorU32(colors::combo::text_active, alpha)), "%s", "[ORTHODOX]");
        ImGui::TextColored(ImColor(GetColorU32(colors::combo::text_active, alpha)), "%s", message.c_str());
        ImGui::Dummy(ImVec2(CalcTextSize(message.c_str()).x + 15, 5));
        PopFont();

        ImGui::End();
    }
};

NotificationSystem notificationSystem;

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_POPUP, 0, 0, 1920, 1080, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    RECT screen_rect;
    GetWindowRect(GetDesktopWindow(), &screen_rect);
    auto x = float(screen_rect.right - 640) / 2.f;
    auto y = float(screen_rect.bottom - 520) / 2.f;
   

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    fonts::inter_font = io.Fonts->AddFontFromMemoryTTF(&inter, sizeof inter, 17, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::inter_font_b = io.Fonts->AddFontFromMemoryTTF(&inter, sizeof inter, 18.5f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::inter_bold_font = io.Fonts->AddFontFromMemoryTTF(&inter_bold, sizeof inter_bold, 20, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::inter_bold_font2 = io.Fonts->AddFontFromMemoryTTF(&inter_bold, sizeof inter_bold, 17, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::inter_bold_font3 = io.Fonts->AddFontFromMemoryTTF(&inter_bold, sizeof inter_bold, 18, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::inter_bold_font4 = io.Fonts->AddFontFromMemoryTTF(&inter_bold, sizeof inter_bold, 16, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::combo_icon_font = io.Fonts->AddFontFromMemoryTTF(&combo_icon, sizeof combo_icon, 15, NULL, io.Fonts->GetGlyphRangesCyrillic());
    fonts::weapon_font = io.Fonts->AddFontFromMemoryTTF(&weapon, sizeof weapon, 15, NULL, io.Fonts->GetGlyphRangesCyrillic());

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(17, 17, 17);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        D3DX11_IMAGE_LOAD_INFO image; ID3DX11ThreadPump* pump{ nullptr };

       
        if (pictures::aim_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &aim, sizeof(aim), &image, pump, &pictures::aim_img, 0);
        if (pictures::misc_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &other, sizeof(other), &image, pump, &pictures::misc_img, 0);
        if (pictures::visual_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &visual, sizeof(visual), &image, pump, &pictures::visual_img, 0);

        if (pictures::silentaim_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &silentaim, sizeof(silentaim), &image, pump, &pictures::silentaim_img, 0);
        if (pictures::trigger_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &trigger, sizeof(trigger), &image, pump, &pictures::trigger_img, 0);


        if (pictures::pen_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &pen, sizeof(pen), &image, pump, &pictures::pen_img, 0);
        if (pictures::world_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, &world, sizeof(world), &image, pump, &pictures::world_img, 0);
        if (pictures::settings_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, setting, sizeof(setting), &image, pump, &pictures::settings_img, 0);
        if (pictures::keyboard_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, keyboard, sizeof(keyboard), &image, pump, &pictures::keyboard_img, 0);
        if (pictures::input_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, inputimg, sizeof(inputimg), &image, pump, &pictures::input_img, 0);
        if (pictures::wat_logo_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, wat, sizeof(wat), &image, pump, &pictures::wat_logo_img, 0);
        if (pictures::fps_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, fps, sizeof(fps), &image, pump, &pictures::fps_img, 0);
        if (pictures::player_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, player, sizeof(player), &image, pump, &pictures::player_img, 0);
        if (pictures::time_img == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, timse, sizeof(timse), &image, pump, &pictures::time_img, 0);

        {



            if (features::watermark) {
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowSize(settings::size_watermark);

          
            ImGui::Begin("watermark", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

            const auto& pos = ImGui::GetWindowPos();
            const auto& draw_list = ImGui::GetWindowDrawList();
            ImGuiStyle* style = &ImGui::GetStyle();
       
            {
               
                style->Colors[ImGuiCol_Border] = colors::menu::watermark_border;

                style->ItemSpacing = ImVec2(0, 5);
                style->WindowPadding = ImVec2(0, 0);
                style->WindowRounding = 8.f;

            }

            // name

            draw_list->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + 477, pos.y + 48), ImGui::GetColorU32(colors::menu::watermark_bg), 4.f);
            draw_list->AddRectFilled(ImVec2(pos.x + 10, pos.y + 10), ImVec2(pos.x + 110, pos.y + 40), ImGui::GetColorU32(colors::menu::watermark_filled), 4.f);
            draw_list->AddImage(pictures::wat_logo_img, ImVec2(pos.x + 20, pos.y + 16), ImVec2(pos.x + 36, pos.y + 32), ImVec2(0, 0), ImVec2(1, 1), ImColor(colors::accent_color));
            draw_list->AddText(fonts::inter_bold_font4, 16.f, ImVec2(pos.x + 45, pos.y + 17), ImColor(80, 80, 80), "ORTHODOX");

            // mc

            draw_list->AddRectFilled(ImVec2(pos.x + 120, pos.y + 10), ImVec2(pos.x + 230, pos.y + 40), ImGui::GetColorU32(colors::menu::watermark_filled), 4.f);
            draw_list->AddImage(pictures::fps_img, ImVec2(pos.x + 130, pos.y + 17), ImVec2(pos.x + 146, pos.y + 33), ImVec2(0, 0), ImVec2(1, 1), ImColor(colors::accent_color));
            draw_list->AddText(fonts::inter_bold_font4, 16.f, ImVec2(pos.x + 161, pos.y + 17), ImColor(80, 80, 80), "LEAK");

            // fps

            draw_list->AddRectFilled(ImVec2(pos.x + 240, pos.y + 10), ImVec2(pos.x + 350, pos.y + 40), ImGui::GetColorU32(colors::menu::watermark_filled), 4.f);
            draw_list->AddImage(pictures::player_img, ImVec2(pos.x + 250, pos.y + 17), ImVec2(pos.x + 266, pos.y + 33), ImVec2(0, 0), ImVec2(1, 1), ImColor(colors::accent_color));
            draw_list->AddText(fonts::inter_bold_font4, 16.f, ImVec2(pos.x + 280, pos.y + 17), ImColor(80, 80, 80), "BY");

            // time

            draw_list->AddRectFilled(ImVec2(pos.x + 360, pos.y + 10), ImVec2(pos.x + 470, pos.y + 40), ImGui::GetColorU32(colors::menu::watermark_filled), 4.f);
            draw_list->AddImage(pictures::time_img, ImVec2(pos.x + 370, pos.y + 17), ImVec2(pos.x + 386, pos.y + 33), ImVec2(0, 0), ImVec2(1, 1), ImColor(colors::accent_color));
            draw_list->AddText(fonts::inter_bold_font4, 16.f, ImVec2(pos.x + 404, pos.y + 17), ImColor(80, 80, 80), "ESO");


            
            ImGui::End();
            }
        
        }
        {
            ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Once);
            ImGui::SetNextWindowSize(settings::size_menu);
            ImGui::Begin("ORTHODOX PASTE", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

            const auto& pos = ImGui::GetWindowPos();
            const auto& draw_list = ImGui::GetWindowDrawList();
            ImGuiStyle* style = &ImGui::GetStyle();
           
            {                
                style->Colors[ImGuiCol_WindowBg] = colors::menu::window_bg;
                style->Colors[ImGuiCol_Border] = colors::menu::border;
                
                style->ItemSpacing = ImVec2(0, 5);
                style->WindowPadding = ImVec2(0, 0);
                style->WindowRounding = 8.f;

            }

            {
                ImGui::SetCursorPos(ImVec2(10, 10));
                ImGui::BeginChild("General Tabs", ImVec2(620, 60), true, ImGuiWindowFlags_NoBackground);

                const auto& pos = ImGui::GetWindowPos();
                const auto& draw_list = ImGui::GetWindowDrawList();


                ImGui::GetStyle().AntiAliasedLines = true;
                ImGui::GetStyle().AntiAliasedLinesUseTex = true;
                ImGui::GetStyle().AntiAliasedFill = true;

                draw_list->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 620, pos.y + 60), ImGui::GetColorU32(menu::general_child), 10.f);


                draw_list->AddText(fonts::inter_bold_font, 20.f, ImVec2(pos.x + 56, pos.y + 20), ImColor(255, 255, 255), "ORTHODOX");

                draw_list->AddRectFilledMultiColor(ImVec2(pos.x + 144, pos.y + 15), ImVec2(pos.x + 145.5f, pos.y + 30), ImGui::GetColorU32(colors::accent_color, 0), ImGui::GetColorU32(colors::accent_color, 0), ImGui::GetColorU32(colors::accent_color), ImGui::GetColorU32(colors::accent_color));
                draw_list->AddRectFilledMultiColor(ImVec2(pos.x + 144, pos.y + 30), ImVec2(pos.x + 145.5f, pos.y + 45), ImGui::GetColorU32(colors::accent_color), ImGui::GetColorU32(colors::accent_color), ImGui::GetColorU32(colors::accent_color, 0), ImGui::GetColorU32(colors::accent_color, 0));

                {
                    {
                        if (misc::tab_count == 1) {
                            misc::tab_width = 92;
                        }
                        else if (misc::tab_count == 2) {
                            misc::tab_width = 96;
                        }
                        else if (misc::tab_count == 3) {
                            misc::tab_width = 92;
                        }
                        else if (misc::tab_count == 4) {
                            misc::tab_width = 91;
                        }
                        else if (misc::tab_count == 5) {
                            misc::tab_width = 92;
                        }
                    }
                    misc::anim_tab = ImLerp(misc::anim_tab, (float)(misc::tab_count * misc::tab_width), ImGui::GetIO().DeltaTime * 15.f);
    
                    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x + 175 + misc::anim_tab, pos.y + 57), ImVec2(pos.x + 235 + misc::anim_tab, pos.y + 60), ImColor(colors::accent_color), 10, ImDrawCornerFlags_Top);
                    ImGui::GetWindowDrawList()->AddShadowRect(ImVec2(pos.x + 175 + misc::anim_tab, pos.y + 57), ImVec2(pos.x + 235 + misc::anim_tab, pos.y + 60), ImGui::GetColorU32(colors::accent_color), 10.f, ImVec2(0, 0), ImDrawCornerFlags_Top, 10.f);


                    ImGui::SetCursorPos(ImVec2(155, 12));
                    ImGui::BeginGroup(); {

                        if (ImGui::Tab("Aimbot", pictures::aim_img, ImVec2(87, 40), 0 == misc::tab_count))
                            misc::tab_count = 0;

                        ImGui::SameLine();

                        if (ImGui::Tab("Visuals", pictures::visual_img, ImVec2(86, 40), 3 == misc::tab_count))
                            misc::tab_count = 1;

                        ImGui::SameLine();

                        if (ImGui::Tab("Config", pictures::misc_img, ImVec2(95, 40), 4 == misc::tab_count))
                            misc::tab_count = 2;


                    }ImGui::EndGroup();
                }

                ImGui::EndChild();
            }

            {
                misc::alpha_child = ImLerp(misc::alpha_child, (misc::tab_count == misc::active_tab_count) ? 1.f : 0.f, 15.f * ImGui::GetIO().DeltaTime);
                if (misc::alpha_child < 0.01f && misc::child_add < 0.01f) misc::active_tab_count = misc::tab_count;

                ImGui::SetCursorPos(ImVec2(10, 80));
                ImGui::BeginChild("Main", ImVec2(725, 440), true, ImGuiWindowFlags_NoBackground);


                ImGui::SetCursorPos(ImVec2(0, 100 - (misc::alpha_child * 100)));

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, misc::alpha_child * style->Alpha);

                if (misc::active_tab_count == 0)
                {                 
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::aim_img, "Damage", ImVec2(304, 270), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {                                                        

                            ImGui::Checkbox("Damage Hack", &features::check1);

                            ImGui::Checkbox("Damage Indicator", &features::check2);

                            ImGui::Checkbox("Hit Logs", &features::check3);

                            ImGui::Checkbox("Log Peek", &features::check4);

                            ImGui::Checkbox("Hide Shots", &features::check5);

                            ImGui::Checkbox("Double Tap", &features::check6);
                          
                        }
                        ImGui::EndChildCustom();

                        ImGui::BeginChildCustom(pictures::aim_img, "Misc", ImVec2(304, 150), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            combo::Combo("Combobox", &features::selectedItem, features::items, IM_ARRAYSIZE(features::items), 2);

                            combo::Combo("Combobox2", &features::selected, features::items_count, IM_ARRAYSIZE(features::items_count), 2);
                        }
                        ImGui::EndChildCustom();

                        ImGui::BeginChildCustom(pictures::aim_img, "Aimbot", ImVec2(304, 190), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImGui::SetCursorPos(ImVec2(10, 50));
                            ImGui::BeginGroup(); {

                                ImGui::ColorEdit4("Accent Color", (float*)&colors::accent_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
                                ImGui::ColorEdit4("Fov Color", (float*)&features::fov_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);

                            } ImGui::EndGroup();
                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();                 

                    ImGui::SameLine(0, 10.f);

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::aim_img, "Accuaracy", ImVec2(304, 220), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {

                            ImGui::Checkbox("Hitchance", &features::check7);

                            ImGui::SliderInt("Hitchance Value", &features::sliderint, 0, 100);

                            ImGui::SliderInt("Damage Value", &features::sliderint2, 0, 100);

                            ImGui::SliderInt("Override Value", &features::sliderint3, 0, 100);

                        }
                        ImGui::EndChildCustom();

                        ImGui::BeginChildCustom(pictures::aim_img, "Exploits", ImVec2(304, 200), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {

                            ImGui::Keybind(pictures::keyboard_img, "Spoofer Name", &features::key, &features::mind, true);

                            ImGui::Keybind(pictures::keyboard_img, "Spoofer Id", &features::key2, &features::mind2, true);

                            ImGui::Keybind(pictures::keyboard_img, "Spoofer Clan Tag", &features::key3, &features::mind3, true);
                            
                        }
                        ImGui::EndChildCustom();

                        ImGui::BeginChildCustom(pictures::aim_img, "Target", ImVec2(304, 190), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            combo::MultiCombo("Hitboxes", features::multi, features::multi_items, 5);
                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();

                }

                else if (misc::active_tab_count == 1)
                {
                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::visual_img, "Esp", ImVec2(304, 240), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImGui::Checkbox("Esp preview", &features::esp_perview);

                            combo::MultiCombo("Esp variables", features::multi_esp, features::multi_preview, 7);

                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();

                    ImGui::SameLine(0, 10.f);

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::visual_img, "Chams", ImVec2(304, 200), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            
                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();                                      

                }

                else if (misc::active_tab_count == 2)
                {

                ImGui::BeginGroup();
                {
                    ImGui::BeginChildCustom(pictures::misc_img, "General", ImVec2(304, 300), false, ImGuiWindowFlags_NoScrollWithMouse);
                    {

                        
                        
                    }
                    ImGui::EndChildCustom();

                    ImGui::BeginChildCustom(pictures::misc_img, "Configs", ImVec2(304, 300), false, ImGuiWindowFlags_NoScrollWithMouse);
                    {

                        ImGui::InputTextWithHint("Text", "Config name", features::input, 64);

                        ImGui::BeginGroup(); {

                            ImGui::Button("Create Config", ImVec2(126, 30));

                            ImGui::SameLine(0, 10);

                            ImGui::Button("Delete Config", ImVec2(126, 30));

                        } ImGui::EndGroup();                      

                        ImGui::BeginGroup(); {

                            ImGui::Button("Load Config", ImVec2(126, 30));

                            ImGui::SameLine(0, 10);

                            ImGui::Button("Save Config", ImVec2(126, 30));

                        } ImGui::EndGroup();

                    }
                    ImGui::EndChildCustom();

                } ImGui::EndGroup();

                ImGui::SameLine(0, 10.f);

                ImGui::BeginGroup();
                {
                    ImGui::BeginChildCustom(pictures::misc_img, "Players", ImVec2(304, 200), false, ImGuiWindowFlags_NoScrollWithMouse);
                    {
                        
                    }
                    ImGui::EndChildCustom();

                    ImGui::BeginChildCustom(pictures::misc_img, "Stats", ImVec2(304, 220), false, ImGuiWindowFlags_NoScrollWithMouse);
                    {
                        ImGui::Button("Add Score Me", ImVec2(ImGui::GetWindowWidth() - 31, 30));

                        ImGui::Button("Add Score Enemy", ImVec2(ImGui::GetWindowWidth() - 31, 30));

                        ImGui::Button("Add Score Team", ImVec2(ImGui::GetWindowWidth() - 31, 30));                      

                    }
                    ImGui::EndChildCustom();

                } ImGui::EndGroup();

                }

                else if (misc::active_tab_count == 3)
                {

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::visual_img, "Esp", ImVec2(304, 240), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImGui::Checkbox("Esp preview", &features::esp_perview);

                            combo::MultiCombo("Esp variables", features::multi_esp, features::multi_preview, 7);

                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();

                    ImGui::SameLine(0, 10.f);

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::visual_img, "Chams", ImVec2(304, 200), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {

                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();


                    }
                else if (misc::active_tab_count == 4)
                {

                    ImGui::BeginGroup();
                    {

                        ImGui::BeginChildCustom(pictures::misc_img, "Configs", ImVec2(304, 430), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {

                            ImGui::InputTextWithHint("Text", "Config name", features::input, 64);

                            ImGui::BeginGroup(); {

                                ImGui::Button("Create Config", ImVec2(126, 30));

                                ImGui::SameLine(0, 10);

                                ImGui::Button("Delete Config", ImVec2(126, 30));

                            } ImGui::EndGroup();

                            ImGui::BeginGroup(); {

                                ImGui::Button("Load Config", ImVec2(126, 30));

                                ImGui::SameLine(0, 10);

                                ImGui::Button("Save Config", ImVec2(126, 30));

                            } ImGui::EndGroup();

                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();

                    ImGui::SameLine(0, 10.f);

                    ImGui::BeginGroup();
                    {
                        ImGui::BeginChildCustom(pictures::misc_img, "Menu Globals", ImVec2(304, 200), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImGui::Keybind(pictures::keyboard_img, "Menu Key", &features::key, &features::mind, true);

                            ImGui::Checkbox("Watermark", &features::watermark);

                            ImGui::Checkbox("Notifications", &features::check3);

                            ImGui::Checkbox("ESP Preview", &features::esp_perview);

                        }
                        ImGui::EndChildCustom();

                        ImGui::BeginChildCustom(pictures::misc_img, "Stats", ImVec2(304, 220), false, ImGuiWindowFlags_NoScrollWithMouse);
                        {
                            ImGui::Button("Add Score Me", ImVec2(ImGui::GetWindowWidth() - 31, 30));

                            ImGui::Button("Add Score Enemy", ImVec2(ImGui::GetWindowWidth() - 31, 30));

                            ImGui::Button("Add Score Team", ImVec2(ImGui::GetWindowWidth() - 31, 30));

                        }
                        ImGui::EndChildCustom();

                    } ImGui::EndGroup();

                    }

                ImGui::PopStyleVar();

                ImGui::Spacing();

                ImGui::EndChild();
            }

          
            ImGui::End();
        }

        {
            features::preview_alpha = ImClamp(features::preview_alpha + (4.f * ImGui::GetIO().DeltaTime * (features::esp_perview ? 1.f : -1.f)), 0.f, 1.f);

            ImGuiStyle* style = &ImGui::GetStyle();

            ImGuiContext& g = *GImGui;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, features::preview_alpha * style->Alpha);

            ImGui::SetNextWindowPos(ImVec2(x + 670, 253), ImGuiCond_Once);

            ImGui::SetNextWindowSize(settings::size_preview);           

            if (features::esp_perview)
            {
                notificationSystem.AddNotification("Notify showed", 2000);
                ImGui::Begin("Esp Preview Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

                const auto& pos = ImGui::GetWindowPos();
                const auto& draw_list = ImGui::GetWindowDrawList();

                {
                    if (features::multi_esp[0] == true) {

                        draw_list->AddRect(ImVec2(pos.x + 50, pos.y + 50), ImVec2(pos.x + 250, pos.y + 350), ImGui::GetColorU32(colors::preview::rect), 4.f);
                       
                    }

                    if (features::multi_esp[1] == true) {

                        draw_list->AddRectFilled(ImVec2(pos.x + 43, pos.y + 50), ImVec2(pos.x + 45, pos.y + 350), ImGui::GetColorU32(colors::accent_color), 4.f);
                        draw_list->AddShadowRect(ImVec2(pos.x + 43, pos.y + 50), ImVec2(pos.x + 45, pos.y + 350), ImGui::GetColorU32(colors::accent_color), 10.f, ImVec2(0, 0), 0, 4.f);

                    }

                    if (features::multi_esp[2] == true) {

                        draw_list->AddRectFilled(ImVec2(pos.x + 50, pos.y + 355), ImVec2(pos.x + 250, pos.y + 357), ImGui::GetColorU32(colors::accent_color), 4.f);
                        draw_list->AddShadowRect(ImVec2(pos.x + 50, pos.y + 355), ImVec2(pos.x + 250, pos.y + 357), ImGui::GetColorU32(colors::accent_color), 10.f, ImVec2(0, 0), 0, 4.f);

                    }


                    if (features::multi_esp[3] == true) {

                        draw_list->AddText(fonts::inter_bold_font2, 17.f, ImVec2(pos.x + 120, pos.y + 28), ImGui::GetColorU32(colors::preview::name), "nickname");
                        
                    }

                    if (features::multi_esp[4] == true) {

                        draw_list->AddText(fonts::inter_bold_font2, 17.f, ImVec2(pos.x + 260, pos.y + 52), ImGui::GetColorU32(colors::preview::distance), "29m");

                    }

                    if (features::multi_esp[5] == true) {

                        draw_list->AddText(fonts::weapon_font, 15.f, ImVec2(pos.x + 133, pos.y + 367), ImGui::GetColorU32(colors::preview::distance), "W");

                    }

                    if (features::multi_esp[6] == true) {

                        draw_list->AddCircleFilled(ImVec2(pos.x + 152, pos.y + 105), 11.f, ImGui::GetColorU32(colors::preview::head), 20.f);

                        draw_list->AddShadowCircle(ImVec2(pos.x + 152, pos.y + 105), 11.f, ImGui::GetColorU32(colors::preview::head), 25.f, ImVec2(0, 0));

                        draw_list->AddLine(ImVec2(pos.x + 152, pos.y + 135), ImVec2(pos.x + 152, pos.y + 240), ImGui::GetColorU32(colors::preview::dice));

                        // left hand

                        draw_list->AddLine(ImVec2(pos.x + 100, pos.y + 180), ImVec2(pos.x + 152, pos.y + 135), ImGui::GetColorU32(colors::preview::dice));
                        
                        // right hand

                        draw_list->AddLine(ImVec2(pos.x + 152, pos.y + 135), ImVec2(pos.x + 204, pos.y + 180), ImGui::GetColorU32(colors::preview::dice));
                        
                        // left leg

                        draw_list->AddLine(ImVec2(pos.x + 100, pos.y + 295), ImVec2(pos.x + 152, pos.y + 240), ImGui::GetColorU32(colors::preview::dice));
                        
                        // right leg

                        draw_list->AddLine(ImVec2(pos.x + 152, pos.y + 240), ImVec2(pos.x + 204, pos.y + 295), ImGui::GetColorU32(colors::preview::dice));
                        

                    }
                }

                ImGui::End();
            }

            ImGui::PopStyleVar();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
