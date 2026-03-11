#include "SettingsScene.h"
#include "TweaksScene.h"
#include "ExtrasScene.h"
#include "ConfigBackupScene.h"
#include "../core/StateMachine.h"
#include "imgui.h"

SettingsScene::SettingsScene(StateMachine& machine) : Scene(machine) {}

void SettingsScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Text("Settings");
    ImGui::Separator();

    // Tweaks Manager (Disabled)
    ImGui::BeginDisabled();
    if (ImGui::Button("Tweaks Manager", ImVec2(-1, 40))) {
        // m_state_machine.push_scene(std::make_unique<TweaksScene>(m_state_machine));
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("This feature is coming soon.");
    }

    // Extras Manager
    if (ImGui::Button("Extras Manager", ImVec2(-1, 40))) {
        m_state_machine.push_scene(std::make_unique<ExtrasScene>(m_state_machine));
    }

    // Config Backup
    if (ImGui::Button("Config Backup / Restore", ImVec2(-1, 40))) {
        m_state_machine.push_scene(std::make_unique<ConfigBackupScene>(m_state_machine));
    }
    
    ImGui::Separator();

    if (ImGui::Button("Back to Main Menu", ImVec2(-1, 40))) {
        m_state_machine.pop_state();
    }

    ImGui::End();
}
