#include "ExtrasScene.h"
#include "ScriptRunner.h"
#include "ScriptRunnerScene.h"
#include "../core/StateMachine.h"
#include "imgui.h"

ExtrasScene::ExtrasScene(StateMachine& machine) : Scene(machine) {}

void ExtrasScene::on_enter() {
    needs_refresh = true;
}

void ExtrasScene::render() {
    if (needs_refresh) {
        needs_refresh = false;
    }

    auto& engine = m_state_machine.get_engine();
    auto& script_manager = *engine.get_script_manager_mut();
    auto extra_scripts = script_manager.get_scripts_by_type(ScriptType::EXTRA);
    const auto& ctx = m_state_machine.get_context();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Extras", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float bottom_bar_height = 50.0f;
    ImGui::BeginChild("MainContent", ImVec2(0, -bottom_bar_height), true);

    ImGui::Text("Extras Manager");
    ImGui::Separator();

    for (auto* script : extra_scripts) {
        ImGui::Text("%s", script->title.c_str());
        ImGui::Indent();
        ImGui::TextDisabled("%s", script->author.c_str());
        ImGui::TextWrapped("%s", script->description.c_str());
        
        fs::path installed_path = script->installed_location;
        if (!installed_path.is_absolute()) {
            installed_path = ctx.path_base / installed_path;
        }

        bool is_installed = fs::exists(installed_path);
        
        if (is_installed) {
            // Show Uninstall and maybe Update buttons
            if (ImGui::Button(("Uninstall##" + script->title).c_str())) {
                auto runner = std::make_shared<UIScriptRunner>(m_state_machine, *script);
                auto scene = std::make_unique<ScriptRunnerScene>(m_state_machine, runner, *script, false, ArgType::UNINSTALL);
                m_state_machine.push_scene(std::move(scene));
            }
            if (script->args_templates.count(ArgType::UPDATE)) {
                ImGui::SameLine();
                if (ImGui::Button(("Update##" + script->title).c_str())) {
                    auto runner = std::make_shared<UIScriptRunner>(m_state_machine, *script);
                    auto scene = std::make_unique<ScriptRunnerScene>(m_state_machine, runner, *script, false, ArgType::UPDATE);
                    m_state_machine.push_scene(std::move(scene));
                }
            }
        } else {
            // Show Install button
            if (ImGui::Button(("Install##" + script->title).c_str())) {
                 auto runner = std::make_shared<UIScriptRunner>(m_state_machine, *script);
                 auto scene = std::make_unique<ScriptRunnerScene>(m_state_machine, runner, *script, false, ArgType::INSTALL);
                 m_state_machine.push_scene(std::move(scene));
            }
        }

        ImGui::Unindent();
        ImGui::Separator();
    }

    ImGui::EndChild();

    if (ImGui::Button("Back", ImVec2(-1, 40))) {
        m_state_machine.pop_state();
    }

    ImGui::End();
}
