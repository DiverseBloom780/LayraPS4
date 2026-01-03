#include "boot_screen.h"
#include <imgui.h>
#include <SDL3/SDL.h>
// Boot screen implementation for LayraPS4
class BootScreen {
private:
    SDL_Texture* logo_texture = nullptr;
    bool show_boot_screen = true;
    float boot_timer = 0.0f;
    const float BOOT_DURATION = 3.0f; // 3 second boot sequence
    
public:
    BootScreen() {
        // Initialize boot screen resources
        LoadLogo();
    }
    
    ~BootScreen() {
        if (logo_texture) {
            SDL_DestroyTexture(logo_texture);
        }
    }
    
    void LoadLogo() {
        // Load the LayraPS4 logo texture
        // This would typically load from resources/layra_logo.png
        // For now, we'll use a placeholder
        ImVec2 logo_size(400, 225); // Scaled logo size
    }
    
    void Render() {
        if (!show_boot_screen) return;
        
        boot_timer += ImGui::GetIO().DeltaTime;
        
        // Full screen boot overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        
        ImGui::Begin("BootScreen", nullptr, 
                    ImGuiWindowFlags_NoDecoration | 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
        
        // Center the logo
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImVec2 logo_size(400, 225);
        ImVec2 logo_pos((display_size.x - logo_size.x) / 2, (display_size.y - logo_size.y) / 2);
        
        // Draw placeholder logo (you'll replace this with actual texture loading)
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(logo_pos, ImVec2(logo_pos.x + logo_size.x, logo_pos.y + logo_size.y), 
                                IM_COL32(0, 100, 255, 255), 10.0f);
        
        // Add "LayraPS4" text below logo
        ImGui::SetCursorPos(ImVec2(logo_pos.x + (logo_size.x - 200) / 2, logo_pos.y + logo_size.y + 20));
        ImGui::PushFont(ImGui::GetFont());
        ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "LayraPS4");
        ImGui::PopFont();
        
        // Boot progress indicator
        float progress = boot_timer / BOOT_DURATION;
        ImGui::SetCursorPos(ImVec2(display_size.x / 2 - 150, display_size.y - 100));
        ImGui::ProgressBar(progress, ImVec2(300, 20));
        
        ImGui::End();
        ImGui::PopStyleVar();
        
        // Auto-hide after boot duration
        if (boot_timer >= BOOT_DURATION) {
            show_boot_screen = false;
        }
    }
    
    bool IsBootComplete() const {
        return !show_boot_screen;
    }
    
    void Reset() {
        boot_timer = 0.0f;
        show_boot_screen = true;
    }
};

// Global boot screen instance
static BootScreen g_boot_screen;

// C interface functions
extern "C" {
    
void boot_screen_init(void) {
    g_boot_screen = BootScreen();
}

void boot_screen_render(void) {
    g_boot_screen.Render();
}

bool boot_screen_is_complete(void) {
    return g_boot_screen.IsBootComplete();
}

void boot_screen_reset(void) {
    g_boot_screen.Reset();
}

} // extern "C"
