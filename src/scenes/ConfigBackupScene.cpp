#include "ConfigBackupScene.h"
#include "../core/StateMachine.h"
#include "../utils/Logger.h"
#include "imgui.h"
#include <map>
#include <regex>

ConfigBackupScene::ConfigBackupScene(StateMachine& machine) : Scene(machine) {}

void ConfigBackupScene::on_enter() {
    find_backups();
}

void ConfigBackupScene::find_backups() {
    m_backups.clear();
    fs::path openmw_dir = m_state_machine.get_context().path_openmw_cfg.parent_path();
    if (!fs::is_directory(openmw_dir)) return;

    std::map<std::string, BackupPair> potential_backups;
    std::regex backup_pattern(R"((options|settings)\.(\d{4}\.\d{2}\.\d{2}-\d{4})\.cfg)");

    for (const auto& entry : fs::directory_iterator(openmw_dir)) {
        std::string filename = entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, backup_pattern) && match.size() == 3) {
            std::string type = match[1].str();
            std::string timestamp = match[2].str();
            
            potential_backups[timestamp].timestamp = timestamp;
            if (type == "options") {
                potential_backups[timestamp].options_file = entry.path();
            } else {
                potential_backups[timestamp].settings_file = entry.path();
            }
        }
    }
    
    // Filter for pairs that have both files
    for (const auto& pair : potential_backups) {
        if (!pair.second.options_file.empty() && !pair.second.settings_file.empty()) {
            m_backups.push_back(pair.second);
        }
    }

    // Sort by timestamp descending (newest first)
    std::sort(m_backups.begin(), m_backups.end(), [](const BackupPair& a, const BackupPair& b) {
        return a.timestamp > b.timestamp;
    });
}

void ConfigBackupScene::restore_backup(const BackupPair& pair) {
    fs::path openmw_dir = m_state_machine.get_context().path_openmw_cfg.parent_path();
    fs::path dest_options = openmw_dir / "options.cfg";
    fs::path dest_settings = openmw_dir / "settings.cfg";

    try {
        LOG_INFO("Restoring backup ", pair.timestamp);
        LOG_INFO("Copying ", pair.options_file, " to ", dest_options);
        fs::copy_file(pair.options_file, dest_options, fs::copy_options::overwrite_existing);
        LOG_INFO("Copying ", pair.settings_file, " to ", dest_settings);
        fs::copy_file(pair.settings_file, dest_settings, fs::copy_options::overwrite_existing);
        LOG_INFO("Restore complete.");
    } catch(const fs::filesystem_error& e) {
        LOG_ERROR("Failed to restore backup: ", e.what());
    }
}

void ConfigBackupScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Backups", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float bottom_bar_height = 50.0f;
    ImGui::BeginChild("MainContent", ImVec2(0, -bottom_bar_height), true);

    ImGui::Text("Config Backups");
    ImGui::Separator();
    
    if (m_backups.empty()) {
        ImGui::TextDisabled("No valid backup pairs found in the OpenMW config directory.");
    }

    for (size_t i = 0; i < m_backups.size(); ++i) {
        if (ImGui::Selectable(m_backups[i].timestamp.c_str(), m_selected_index == (int)i)) {
            m_selected_index = i;
        }
    }
    
    ImGui::EndChild();

    if (m_selected_index >= 0) {
        if (ImGui::Button("Restore Selected", ImVec2(ImGui::GetContentRegionAvail().x / 2, 40))) {
            restore_backup(m_backups[m_selected_index]);
        }
        ImGui::SameLine();
    }
    
    if (ImGui::Button("Back", ImVec2(-1, 40))) {
        m_state_machine.pop_state();
    }

    ImGui::End();
}
