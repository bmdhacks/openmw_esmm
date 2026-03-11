#pragma once
#include "Scene.h"
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

struct BackupPair {
    std::string timestamp;
    fs::path options_file;
    fs::path settings_file;
};

class ConfigBackupScene : public Scene {
public:
    ConfigBackupScene(StateMachine& machine);
    void on_enter() override;
    void render() override;
private:
    void find_backups();
    void restore_backup(const BackupPair& pair);

    std::vector<BackupPair> m_backups;
    int m_selected_index = -1;
};