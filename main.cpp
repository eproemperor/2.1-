#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

enum class RobotType : uint16_t {
    kInfantry = 0,
    kEngineer = 1
};

enum class RobotStatus : uint16_t {
    kNormal = 0,
    kDestroyed = 1
};

enum class InfantryLevel : uint16_t {
    kLevel1 = 1,
    kLevel2 = 2,
    kLevel3 = 3
};

struct InputCommand {
    uint16_t time;
    char command_word;
    uint16_t belonging_team;
    uint16_t robot_logo;
    uint16_t func_instruct;
};

class Robot {
public:
    Robot(uint16_t team_id, uint16_t robot_id, RobotType type)
        : team_id_(team_id),
        robot_id_(robot_id),
        type_(type),
        status_(RobotStatus::kNormal),
        max_hp_(0),
        max_heat_(0),
        current_hp_(0),
        current_heat_(0) {
    }

    virtual ~Robot() = default;

    virtual void TakeDamage(uint16_t damage) = 0;
    virtual void AddHeat(uint16_t heat) = 0;
    virtual bool CanUpgrade(uint16_t target_level) const = 0;
    virtual void Upgrade(uint16_t target_level) = 0;

    virtual void Update(int time_passed) {
        if (IsDestroyed() || time_passed <= 0) {
            return;
        }
        for (int i = 0; i < time_passed; ++i) {
            if (current_heat_ > 0) {
                --current_heat_;
            }
            if (max_heat_ > 0 && current_heat_ > max_heat_) {
                TakeDamage(1);
            }
            if (current_hp_ <= 0) {
                CheckIfDestroyed();
                break;
            }
        }
    }

    void Reset() {
        current_hp_ = max_hp_;
        current_heat_ = 0;
        status_ = RobotStatus::kNormal;
    }

    void Revive() {
        Reset();
    }

    uint16_t GetTeamId() const { return team_id_; }
    uint16_t GetRobotId() const { return robot_id_; }
    uint16_t GetCurrentHp() const { return current_hp_; }
    uint16_t GetCurrentHeat() const { return current_heat_; }
    uint16_t GetMaxHp() const { return max_hp_; }
    uint16_t GetMaxHeat() const { return max_heat_; }
    RobotType GetType() const { return type_; }
    RobotStatus GetStatus() const { return status_; }
    bool IsDestroyed() const { return status_ == RobotStatus::kDestroyed; }
    bool IsAlive() const { return current_hp_ > 0; }
    void SetStatus(RobotStatus status) { status_ = status; }

protected:
    uint16_t team_id_;
    uint16_t robot_id_;
    RobotType type_;
    RobotStatus status_;
    uint16_t max_hp_;
    uint16_t max_heat_;
    uint16_t current_hp_;
    uint16_t current_heat_;

    void CheckIfDestroyed() {
        if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
            status_ = RobotStatus::kDestroyed;
        }
    }
};

class InfantryRobot : public Robot {
public:
    InfantryRobot(uint16_t team_id, uint16_t robot_id, InfantryLevel level)
        : Robot(team_id, robot_id, RobotType::kInfantry),
        level_(level) {
        UpdateStatsForLevel(level);
        Reset();
    }

    void TakeDamage(uint16_t damage) override {
        if (damage >= current_hp_) {
            current_hp_ = 0;
        }
        else {
            current_hp_ -= damage;
        }
    }

    void AddHeat(uint16_t heat) override {
        uint32_t new_heat = static_cast<uint32_t>(current_heat_) + heat;
        current_heat_ = static_cast<uint16_t>(std::min(new_heat, static_cast<uint32_t>(UINT16_MAX)));
    }

    bool CanUpgrade(uint16_t target_level) const override {
        return (target_level > static_cast<uint16_t>(level_)) &&
            (target_level >= 1 && target_level <= 3);
    }

    void Upgrade(uint16_t target_level) override {
        if (!CanUpgrade(target_level)) {
            return;
        }
        InfantryLevel new_level = static_cast<InfantryLevel>(target_level);
        level_ = new_level;
        UpdateStatsForLevel(new_level);
        current_hp_ = max_hp_;
        current_heat_ = 0;
    }

    void Update(int time_passed) override {
        if (IsDestroyed() || time_passed <= 0) {
            return;
        }
        for (int i = 0; i < time_passed; ++i) {
            if (current_heat_ > 0) {
                --current_heat_;
            }
            if (max_heat_ > 0 && current_heat_ > max_heat_) {
                TakeDamage(1);
            }
            if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
                status_ = RobotStatus::kDestroyed;
            }
            if (IsDestroyed()) {
                continue;
            }
        }
    }

    InfantryLevel GetLevel() const { return level_; }

    static uint16_t GetHpForLevel(InfantryLevel level) {
        switch (level) {
        case InfantryLevel::kLevel1: return 100;
        case InfantryLevel::kLevel2: return 150;
        case InfantryLevel::kLevel3: return 250;
        default: return 100;
        }
    }

    static uint16_t GetHeatForLevel(InfantryLevel level) {
        switch (level) {
        case InfantryLevel::kLevel1: return 100;
        case InfantryLevel::kLevel2: return 200;
        case InfantryLevel::kLevel3: return 300;
        default: return 100;
        }
    }

private:
    InfantryLevel level_;

    void UpdateStatsForLevel(InfantryLevel level) {
        max_hp_ = GetHpForLevel(level);
        max_heat_ = GetHeatForLevel(level);
    }
};

class EngineerRobot : public Robot {
public:
    EngineerRobot(uint16_t team_id, uint16_t robot_id)
        : Robot(team_id, robot_id, RobotType::kEngineer) {
        max_hp_ = 300;
        max_heat_ = 0;
        Reset();
    }

    void TakeDamage(uint16_t damage) override {
        if (damage >= current_hp_) {
            current_hp_ = 0;
        }
        else {
            current_hp_ -= damage;
        }
    }

    void AddHeat(uint16_t heat) override {}

    bool CanUpgrade(uint16_t target_level) const override {
        return false;
    }

    void Upgrade(uint16_t target_level) override {}

    void Update(int time_passed) override {
        if (IsDestroyed() || time_passed <= 0) {
            return;
        }
        if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
            status_ = RobotStatus::kDestroyed;
        }
    }
};

class RobotManage {
public:
    RobotManage() : present_time_(0), last_time_(0) {}

    void Run() {
        int n;
        std::cin >> n;
        std::string dummy;
        std::getline(std::cin, dummy);
        for (int i = 0; i < n; ++i) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                break;
            }
            if (line.empty()) {
                --i;
                continue;
            }
            InputCommand cmd = ParseCommandLine(line);
            ProcessCommand(cmd);
        }
    }

private:
    std::vector<std::shared_ptr<Robot>> normal_robot_list_;
    std::vector<std::shared_ptr<Robot>> destroyed_robot_list_;
    uint16_t present_time_;
    uint16_t last_time_;

    InputCommand ParseCommandLine(const std::string& line) {
        InputCommand cmd;
        std::istringstream iss(line);
        iss >> cmd.time >> cmd.command_word >> cmd.belonging_team
            >> cmd.robot_logo >> cmd.func_instruct;
        return cmd;
    }

    void ProcessCommand(const InputCommand& cmd) {
        UpdateTime(cmd.time);
        switch (cmd.command_word) {
        case 'A':
            AddRobot(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'F':
            CauseDamage(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'H':
            IncreaseHeat(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'U':
            UpgradeRobot(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        }
        last_time_ = cmd.time;
    }

    void UpdateTime(uint16_t current_time) {
        if (current_time <= last_time_) {
            return;
        }
        uint16_t time_passed = current_time - last_time_;
        std::vector<std::shared_ptr<Robot>> to_destroy;
        for (auto& robot : normal_robot_list_) {
            robot->Update(time_passed);
            if (robot->GetCurrentHp() <= 0 && !robot->IsDestroyed()) {
                to_destroy.push_back(robot);
            }
        }
        for (auto& robot : to_destroy) {
            robot->SetStatus(RobotStatus::kDestroyed);
            OutputDestroyed(robot->GetTeamId(), robot->GetRobotId());
            destroyed_robot_list_.push_back(robot);
            normal_robot_list_.erase(
                std::remove(normal_robot_list_.begin(), normal_robot_list_.end(), robot),
                normal_robot_list_.end());
        }
    }

    std::shared_ptr<Robot> FindRobot(uint16_t team, uint16_t id,
        const std::vector<std::shared_ptr<Robot>>& list) {
        for (const auto& robot : list) {
            if (robot->GetTeamId() == team && robot->GetRobotId() == id) {
                return robot;
            }
        }
        return nullptr;
    }

    void AddRobot(uint16_t team, uint16_t id, uint16_t type) {
        auto robot = FindRobot(team, id, destroyed_robot_list_);
        if (robot) {
            RobotType robot_type = static_cast<RobotType>(type);
            if (robot->GetType() == robot_type) {
                robot->Reset();
                robot->SetStatus(RobotStatus::kNormal);
                normal_robot_list_.push_back(robot);
                destroyed_robot_list_.erase(
                    std::remove(destroyed_robot_list_.begin(), destroyed_robot_list_.end(), robot),
                    destroyed_robot_list_.end());
                return;
            }
        }
        robot = FindRobot(team, id, normal_robot_list_);
        if (robot) {
            return;
        }
        RobotType robot_type = static_cast<RobotType>(type);
        if (robot_type == RobotType::kInfantry) {
            robot = std::make_shared<InfantryRobot>(team, id, InfantryLevel::kLevel1);
        }
        else {
            robot = std::make_shared<EngineerRobot>(team, id);
        }
        normal_robot_list_.push_back(robot);
    }

    void CauseDamage(uint16_t team, uint16_t id, uint16_t damage) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;
        }
        robot->TakeDamage(damage);
        if (robot->GetCurrentHp() <= 0 && robot->GetStatus() != RobotStatus::kDestroyed) {
            robot->SetStatus(RobotStatus::kDestroyed);
            OutputDestroyed(team, id);
            destroyed_robot_list_.push_back(robot);
            normal_robot_list_.erase(
                std::remove(normal_robot_list_.begin(), normal_robot_list_.end(), robot),
                normal_robot_list_.end());
        }
    }

    void IncreaseHeat(uint16_t team, uint16_t id, uint16_t heat) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;
        }
        if (robot->GetType() == RobotType::kInfantry) {
            robot->AddHeat(heat);
        }
    }

    void UpgradeRobot(uint16_t team, uint16_t id, uint16_t target_level) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;
        }
        if (robot->GetType() != RobotType::kInfantry) {
            return;
        }
        if (robot->CanUpgrade(target_level)) {
            robot->Upgrade(target_level);
        }
    }

    void OutputDestroyed(uint16_t team, uint16_t id) {
        std::cout << "D " << team << " " << id << std::endl;
    }
};

int main() {
    RobotManage manager;
    manager.Run();
    return 0;
}