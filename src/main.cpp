// LayraPS4 – PS4 OS Emulator (error-free build) main.cpp
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDLvulkan.h>
#include "imgui.h"
#include "imguiimplsdl3.h"
#include "imguiimplvulkan.h"
// Stubs (replace with real implementations later)
namespace orbis {
    void audioplaybootsound() {
        // Load the boot sound file
        SDLAudioSpec spec;
        Uint32 wavlength;
        Uint8 wavbuffer;
        SDLLoadWAV("bootsound.wav", &spec, &wavbuffer, &wavlength);

        // Play the sound
        SDLAudioDeviceID device = SDLOpenAudioDevice(NULL, 0, &spec, NULL, 0);
        SDLQueueAudio(device, wavbuffer, wavlength);
        SDLPauseAudioDevice(device, 0);
    }

    void kernelinit(void arg) {
        // Initialize kernel memory management
        //kernelmemoryinit();

        // Initialize kernel threads
        //kernelthreadinit();
    }

    void modulesinit() {
        // Load the necessary modules
        //moduleload("module1");
        //moduleload("module2");

        // Initialize module functionality
        //moduleinit("module1");
        //moduleinit("module2");
    }

    void audioinit() {
        // Initialize audio subsystem
        //audiosubsysteminit();

        // Set up audio devices
        //audiodevicesetup();
    }

    void padinit() {
        // Initialize pad subsystem
        //padsubsysteminit();

        // Set up pad devices
        //paddevicesetup();
    }

    void savedatainit() {
        // Initialize savedata subsystem
        //savedatasubsysteminit();

        // Set up savedata devices
        //savedatadevicesetup();
    }

    void trophyinit() {
        // Initialize trophy subsystem
        //trophysubsysteminit();

        // Set up trophy devices
        //trophydevicesetup();
    }

    void kernelshutdown(void arg) {
        // Shutdown kernel memory management
        kernelmemoryshutdown();

        // Shutdown kernel threads
        kernelthreadshutdown();

        // Release any resources used by the kernel
        // ...
    }
}

#include "layrapkg.h"
#include "layravulkan.h"

struct Theme {
    std::string name;
    ImVec4 backgroundColor;
    ImVec4 buttonColor;
    ImVec4 buttonHoverColor;
};

class ThemeManager {
public:
    ThemeManager() {}
    ~ThemeManager() {}

    void addTheme(const Theme& theme) {
        themes.pushback(theme);
    }

    void selectTheme(const std::string& themeName) {
        for (const auto& theme : themes) {
            if (theme.name == themeName) {
                currentTheme = theme;
                break;
            }
        }
    }

    const Theme& getCurrentTheme() const {
        return currentTheme;
    }

private:
    std::vector<Theme> themes;
    Theme currentTheme;
};

static VkDescriptorPool gDescriptorPool = VKNULLHANDLE;
ThemeManager themeManager;

void ImGuiRenderCallback(VkCommandBuffer cmd) {
    ImGuiImplVulkanRenderDrawData(ImGui::GetDrawData(), cmd);
}

// PS4 Boot Sequence
void RenderPS4BootSequence(ImGuiIO& io) {
    static Uint64 start = SDLGetTicks();
    Uint64 now = SDLGetTicks();
    float alpha = (now - start) < 2000 ? 1.0f : 0.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiColWindowBg, ImVec4(0, 0, 0, alpha));

    ImGui::Begin("Boot", nullptr,
                 ImGuiWindowFlagsNoDecoration | ImGuiWindowFlagsNoMove);
    ImVec2 txt = ImGui::CalcTextSize("LayraPS4 - Booting Orbis OS...");
    ImGui::SetCursorPos((io.DisplaySize - txt)  0.5f);
    ImGui::Text("LayraPS4 - Booting Orbis OS...");
    ImGui::End();
    ImGui::PopStyleColor();
}

// PS4 Dashboard
void RenderPS4Dashboard(ImGuiIO& io) {
    themeManager.addTheme(Theme{"Default", ImVec4(0.06f, 0.08f, 0.12f, 0.95f), ImVec4(0.16f, 0.29f, 0.48f, 0.40f), ImVec4(0.26f, 0.59f, 0.98f, 1.00f)});
    themeManager.addTheme(Theme{"Dark", ImVec4(0.02f, 0.02f, 0.02f, 0.95f), ImVec4(0.08f, 0.08f, 0.08f, 0.40f), ImVec4(0.12f, 0.12f, 0.12f, 1.00f)});
    themeManager.selectTheme("Default");

    const Theme& currentTheme = themeManager.getCurrentTheme();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleColor(ImGuiColWindowBg, currentTheme.backgroundColor);
    ImGui::PushStyleVar(ImGuiStyleVarWindowPadding, ImVec2(0, 0));

    ImGui::Begin("PS4 Dashboard", nullptr,
                 ImGuiWindowFlagsNoTitleBar 
//ImGuiWindowFlagsNoResize
                 ImGuiWindowFlagsNoMove | ImGuiWindowFlagsNoCollapse);
    // Top row – function icons
    ImGui::Indent(60);
    const char icons[] = { "Store", "Friends", "Settings", "Power" };
    for (int i = 0; i < 4; ++i) {
        if (ImGui::Button(icons[i], ImVec2(120, 40))) {}
        ImGui::SameLine(0, 20);
    }

    // Middle row – game tiles (hard-coded for now)
    float contentY = io.DisplaySize.y  0.35f;
    ImGui::SetCursorPos(ImVec2(100, contentY));
    const char games[] = { "Bloodborne", "Playroom", "The Last of Us Part II" };
    for (int i = 0; i < 3; ++i) {
        ImGui::BeginGroup();
        if (ImGui::Button(games[i], ImVec2(240, 240))) {}
        ImGui::Text("%s", games[i]);
        ImGui::EndGroup();
        ImGui::SameLine(0, 30);
    }

    // Theme selection menu
    if (ImGui::BeginMenu("Themes")) {
        for (const auto& theme : themeManager.themes) {
            if (ImGui::MenuItem(theme.name.cstr())) {
                themeManager.selectTheme(theme.name);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/ ----------  Main  ---------- /
int main(int, char[]) {
    if (SDLInit(SDLINITVIDEO 
//SDLINITTIMER	SDLINITGAMEPAD
 SDLINITAUDIO) != 0) {
        std::printf("SDLInit failed: %s\n", SDLGetError());
        return -1;
    }
    SDLWindow window = SDLCreateWindow(
        "LayraPS4 - PS4 OS Emulator", 1920, 1080,
        SDLWINDOWVULKAN 
//SDLWINDOWRESIZABLE
 SDLWINDOWHIGHPIXELDENSITY);
    if (!window) return -1;
    LayraVulkanContext vk{};
    if (!layravulkaninit(&vk, window)) {
        SDLDestroyWindow(window);
        SDLQuit();
        return -1;
    }

    // ImGui setup
    IMGUICHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags 
//= ImGuiConfigFlagsNavEnableKeyboard
// ImGuiConfigFlagsNavEnableGamepad;
    // PS4 dark theme
    ImVec4 c = ImGui::GetStyle().Colors;
    c[ImGuiColWindowBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.95f);
    c[ImGuiColButton] = ImVec4(0.16f, 0.29f, 0.48f, 0.40f);
    c[ImGuiColButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Descriptor pool
    VkDescriptorPoolSize poolsizes[] = {
        { VKDESCRIPTORTYPESAMPLER, 1000 },
        { VKDESCRIPTORTYPECOMBINEDIMAGESAMPLER, 1000 },
        { VKDESCRIPTORTYPESAMPLEDIMAGE, 1000 },
        { VKDESCRIPTORTYPESTORAGEIMAGE, 1000 },
        { VKDESCRIPTORTYPEUNIFORMBUFFER, 1000 },
        { VKDESCRIPTORTYPESTORAGEBUFFER, 1000 }
    };
    VkDescriptorPoolCreateInfo poolinfo{
        .sType = VKSTRUCTURETYPEDESCRIPTORPOOLCREATEINFO,
        .flags = VKDESCRIPTORPOOLCREATEFREEDESCRIPTORSETBIT,
        .maxSets = 1000,
        .poolSizeCount = (uint32t)IM
