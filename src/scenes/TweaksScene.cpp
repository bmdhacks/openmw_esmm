// TweaksScene.cpp
#include "TweaksScene.h"
#include "../core/StateMachine.h"
#include "imgui.h"

TweaksScene::TweaksScene(StateMachine& machine) : Scene(machine) {}

void TweaksScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Tweaks", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::Text("Tweaks Manager (Coming Soon)");
    ImGui::Separator();

    if (ImGui::Button("Back", ImVec2(-1, 40))) {
        m_state_machine.pop_state();
    }
    ImGui::End();
}
