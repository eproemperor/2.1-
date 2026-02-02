#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// 机器人类型枚举
enum class RobotType : uint16_t {
    kInfantry = 0,  // 步兵机器人
    kEngineer = 1   // 工程机器人
};

// 机器人状态枚举
enum class RobotStatus : uint16_t {
    kNormal = 0,    // 正常状态
    kDestroyed = 1  // 被击毁状态
};

// 步兵等级枚举
enum class InfantryLevel : uint16_t {
    kLevel1 = 1,    // 等级1
    kLevel2 = 2,    // 等级2
    kLevel3 = 3     // 等级3
};

// 输入命令结构体
struct InputCommand {
    uint16_t time;           // 时间戳
    char command_word;       // 命令字符（A/F/H/U）
    uint16_t belonging_team; // 所属队伍
    uint16_t robot_logo;     // 机器人标识符
    uint16_t func_instruct;  // 功能指令（类型/伤害值/热量值/目标等级）
};

// 抽象基类：机器人
class Robot {
public:
    // 构造函数
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

    // 纯虚函数接口
    virtual void TakeDamage(uint16_t damage) = 0;               // 承受伤害
    virtual void AddHeat(uint16_t heat) = 0;                    // 增加热量
    virtual bool CanUpgrade(uint16_t target_level) const = 0;   // 检查是否可以升级
    virtual void Upgrade(uint16_t target_level) = 0;            // 升级机器人

    // 时间更新：计算热量衰减和超热伤害
    virtual void Update(int time_passed) {
        if (IsDestroyed() || time_passed <= 0) {
            return;  // 已击毁或时间无效则直接返回
        }
        // 逐秒计算热量衰减和超热伤害
        for (int i = 0; i < time_passed; ++i) {
            // 热量衰减：每秒减少1点
            if (current_heat_ > 0) {
                --current_heat_;
            }
            // 超热伤害：热量超过上限时每秒造成1点伤害
            if (max_heat_ > 0 && current_heat_ > max_heat_) {
                TakeDamage(1);
            }
            // 血量归零则标记为击毁并停止后续时间更新
            if (current_hp_ <= 0) {
                CheckIfDestroyed();
                break;
            }
        }
    }

    // 重置状态：恢复血量和热量
    void Reset() {
        current_hp_ = max_hp_;
        current_heat_ = 0;
        status_ = RobotStatus::kNormal;
    }

    // 恢复机器人（同Reset）
    void Revive() {
        Reset();
    }

    // Getter方法
    uint16_t GetTeamId() const { return team_id_; }            // 获取队伍ID
    uint16_t GetRobotId() const { return robot_id_; }          // 获取机器人ID
    uint16_t GetCurrentHp() const { return current_hp_; }      // 获取当前血量
    uint16_t GetCurrentHeat() const { return current_heat_; }  // 获取当前热量
    uint16_t GetMaxHp() const { return max_hp_; }              // 获取血量上限
    uint16_t GetMaxHeat() const { return max_heat_; }          // 获取热量上限
    RobotType GetType() const { return type_; }                // 获取机器人类型
    RobotStatus GetStatus() const { return status_; }          // 获取机器人状态
    bool IsDestroyed() const { return status_ == RobotStatus::kDestroyed; }  // 是否被击毁
    bool IsAlive() const { return current_hp_ > 0; }            // 是否存活
    void SetStatus(RobotStatus status) { status_ = status; }    // 设置状态

protected:
    uint16_t team_id_;           // 所属队伍ID
    uint16_t robot_id_;          // 机器人唯一标识符
    RobotType type_;             // 机器人类型（步兵/工程）
    RobotStatus status_;         // 当前状态（正常/被击毁）
    uint16_t max_hp_;            // 血量上限
    uint16_t max_heat_;          // 热量上限
    uint16_t current_hp_;        // 当前血量
    uint16_t current_heat_;      // 当前热量

    // 检查是否需要击毁（血量<=0时标记为击毁状态）
    void CheckIfDestroyed() {
        if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
            status_ = RobotStatus::kDestroyed;
        }
    }
};

// 步兵机器人类
class InfantryRobot : public Robot {
public:
    // 构造函数：初始化步兵机器人
    InfantryRobot(uint16_t team_id, uint16_t robot_id, InfantryLevel level)
        : Robot(team_id, robot_id, RobotType::kInfantry),
        level_(level) {
        UpdateStatsForLevel(level);  // 根据等级设置属性
        Reset();  // 初始化血量和热量
    }

    // 承受伤害实现
    void TakeDamage(uint16_t damage) override {
        if (damage >= current_hp_) {
            current_hp_ = 0;  // 伤害大于等于当前血量，直接归零
        }
        else {
            current_hp_ -= damage;  // 扣除相应血量
        }
    }

    // 增加热量实现（防止溢出）
    void AddHeat(uint16_t heat) override {
        uint32_t new_heat = static_cast<uint32_t>(current_heat_) + heat;
        current_heat_ = static_cast<uint16_t>(std::min(new_heat, static_cast<uint32_t>(UINT16_MAX)));
    }

    // 检查是否可以升级：只能升级到更高等级（1→2→3）
    bool CanUpgrade(uint16_t target_level) const override {
        return (target_level > static_cast<uint16_t>(level_)) &&
            (target_level >= 1 && target_level <= 3);
    }

    // 升级实现
    void Upgrade(uint16_t target_level) override {
        if (!CanUpgrade(target_level)) {
            return;  // 不能升级则直接返回
        }
        // 更新等级和属性
        InfantryLevel new_level = static_cast<InfantryLevel>(target_level);
        level_ = new_level;
        UpdateStatsForLevel(new_level);
        // 重置血量和热量到新等级的最大值
        current_hp_ = max_hp_;
        current_heat_ = 0;
    }

    // 步兵特殊时间更新：考虑被击毁后的热量衰减
    void Update(int time_passed) override {
        if (IsDestroyed() || time_passed <= 0) {
            return;
        }
        for (int i = 0; i < time_passed; ++i) {
            // 热量衰减
            if (current_heat_ > 0) {
                --current_heat_;
            }
            // 超热伤害
            if (max_heat_ > 0 && current_heat_ > max_heat_) {
                TakeDamage(1);
            }
            // 血量归零则标记为击毁
            if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
                status_ = RobotStatus::kDestroyed;
            }
            // 被击毁后只进行热量衰减，不再受到伤害
            if (IsDestroyed()) {
                continue;
            }
        }
    }

    // Getter方法
    InfantryLevel GetLevel() const { return level_; }  // 获取当前等级

    // 静态方法：根据等级获取血量
    static uint16_t GetHpForLevel(InfantryLevel level) {
        switch (level) {
        case InfantryLevel::kLevel1: return 100;
        case InfantryLevel::kLevel2: return 150;
        case InfantryLevel::kLevel3: return 250;
        default: return 100;
        }
    }

    // 静态方法：根据等级获取热量上限
    static uint16_t GetHeatForLevel(InfantryLevel level) {
        switch (level) {
        case InfantryLevel::kLevel1: return 100;
        case InfantryLevel::kLevel2: return 200;
        case InfantryLevel::kLevel3: return 300;
        default: return 100;
        }
    }

private:
    InfantryLevel level_;  // 步兵等级

    // 根据等级更新属性（血量和热量上限）
    void UpdateStatsForLevel(InfantryLevel level) {
        max_hp_ = GetHpForLevel(level);
        max_heat_ = GetHeatForLevel(level);
    }
};

// 工程机器人类
class EngineerRobot : public Robot {
public:
    // 构造函数：初始化工程机器人（固定属性）
    EngineerRobot(uint16_t team_id, uint16_t robot_id)
        : Robot(team_id, robot_id, RobotType::kEngineer) {
        max_hp_ = 300;     // 固定血量300
        max_heat_ = 0;     // 没有热量上限
        Reset();
    }

    // 承受伤害实现
    void TakeDamage(uint16_t damage) override {
        if (damage >= current_hp_) {
            current_hp_ = 0;
        }
        else {
            current_hp_ -= damage;
        }
    }

    // 增加热量实现：工程机器人不受热量影响
    void AddHeat(uint16_t heat) override {
        // 工程机器人没有热量系统，什么都不做
    }

    // 检查是否可以升级：工程机器人不能升级
    bool CanUpgrade(uint16_t target_level) const override {
        return false;
    }

    // 升级实现：工程机器人不能升级
    void Upgrade(uint16_t target_level) override {
        // 空实现
    }

    // 时间更新：工程机器人只需要检查是否被击毁
    void Update(int time_passed) override {
        if (IsDestroyed() || time_passed <= 0) {
            return;
        }
        // 血量归零则标记为击毁
        if (current_hp_ <= 0 && status_ != RobotStatus::kDestroyed) {
            status_ = RobotStatus::kDestroyed;
        }
    }
};

// 机器人管理类
class RobotManage {
public:
    RobotManage() : present_time_(0), last_time_(0) {}

    // 主运行函数：读取命令数量并处理所有命令
    void Run() {
        int n;
        std::cin >> n;  // 读取命令数量
        std::string dummy;
        std::getline(std::cin, dummy);  // 清除输入缓冲区
        for (int i = 0; i < n; ++i) {
            std::string line;
            // 读取命令行，遇到EOF提前退出
            if (!std::getline(std::cin, line)) {
                break;
            }
            // 跳过空行
            if (line.empty()) {
                --i;
                continue;
            }
            InputCommand cmd = ParseCommandLine(line);  // 解析命令
            ProcessCommand(cmd);  // 处理命令
        }
    }

private:
    std::vector<std::shared_ptr<Robot>> normal_robot_list_;    // 正常机器人列表
    std::vector<std::shared_ptr<Robot>> destroyed_robot_list_; // 被击毁机器人列表
    uint16_t present_time_;                                    // 当前时间
    uint16_t last_time_;                                       // 上次更新时间

    // 解析命令行字符串为InputCommand结构体
    InputCommand ParseCommandLine(const std::string& line) {
        InputCommand cmd;
        std::istringstream iss(line);
        iss >> cmd.time >> cmd.command_word >> cmd.belonging_team
            >> cmd.robot_logo >> cmd.func_instruct;
        return cmd;
    }

    // 处理单个命令
    void ProcessCommand(const InputCommand& cmd) {
        UpdateTime(cmd.time);  // 首先更新时间状态
        // 根据命令字符执行相应操作
        switch (cmd.command_word) {
        case 'A':  // 添加/恢复机器人
            AddRobot(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'F':  // 造成伤害
            CauseDamage(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'H':  // 增加热量
            IncreaseHeat(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        case 'U':  // 升级机器人
            UpgradeRobot(cmd.belonging_team, cmd.robot_logo, cmd.func_instruct);
            break;
        }
        last_time_ = cmd.time;  // 更新上次处理时间
    }

    // 更新时间：计算从上次更新到当前时间的热量衰减和伤害
    void UpdateTime(uint16_t current_time) {
        if (current_time <= last_time_) {
            return;  // 时间未前进则无需更新
        }
        uint16_t time_passed = current_time - last_time_;  // 计算经过的时间
        std::vector<std::shared_ptr<Robot>> to_destroy;    // 待击毁机器人列表
        // 更新所有正常机器人的状态
        for (auto& robot : normal_robot_list_) {
            robot->Update(time_passed);  // 调用机器人的时间更新
            // 收集血量归零但尚未标记为击毁的机器人
            if (robot->GetCurrentHp() <= 0 && !robot->IsDestroyed()) {
                to_destroy.push_back(robot);
            }
        }
        // 统一处理被击毁的机器人
        for (auto& robot : to_destroy) {
            robot->SetStatus(RobotStatus::kDestroyed);
            OutputDestroyed(robot->GetTeamId(), robot->GetRobotId());  // 输出击毁信息
            destroyed_robot_list_.push_back(robot);  // 移到击毁列表
            // 从正常列表中移除
            normal_robot_list_.erase(
                std::remove(normal_robot_list_.begin(), normal_robot_list_.end(), robot),
                normal_robot_list_.end());
        }
    }

    // 在指定列表中查找机器人
    std::shared_ptr<Robot> FindRobot(uint16_t team, uint16_t id,
        const std::vector<std::shared_ptr<Robot>>& list) {
        for (const auto& robot : list) {
            if (robot->GetTeamId() == team && robot->GetRobotId() == id) {
                return robot;  // 找到匹配的机器人
            }
        }
        return nullptr;  // 未找到
    }

    // 添加机器人命令处理
    void AddRobot(uint16_t team, uint16_t id, uint16_t type) {
        // 1. 先在击毁列表中查找（尝试恢复）
        auto robot = FindRobot(team, id, destroyed_robot_list_);
        if (robot) {
            RobotType robot_type = static_cast<RobotType>(type);
            // 类型匹配才能恢复
            if (robot->GetType() == robot_type) {
                robot->Reset();
                robot->SetStatus(RobotStatus::kNormal);
                normal_robot_list_.push_back(robot);  // 移回正常列表
                // 从击毁列表中移除
                destroyed_robot_list_.erase(
                    std::remove(destroyed_robot_list_.begin(), destroyed_robot_list_.end(), robot),
                    destroyed_robot_list_.end());
                return;  // 恢复成功，直接返回
            }
        }
        // 2. 检查是否已存在于正常列表中
        robot = FindRobot(team, id, normal_robot_list_);
        if (robot) {
            return;  // 已存在，什么也不做
        }
        // 3. 创建新机器人
        RobotType robot_type = static_cast<RobotType>(type);
        if (robot_type == RobotType::kInfantry) {
            robot = std::make_shared<InfantryRobot>(team, id, InfantryLevel::kLevel1);  // 创建1级步兵
        }
        else {
            robot = std::make_shared<EngineerRobot>(team, id);  // 创建工程机器人
        }
        normal_robot_list_.push_back(robot);  // 添加到正常列表
    }

    // 造成伤害命令处理
    void CauseDamage(uint16_t team, uint16_t id, uint16_t damage) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;  // 机器人不存在
        }
        robot->TakeDamage(damage);  // 施加伤害
        // 立即检查是否需要击毁
        if (robot->GetCurrentHp() <= 0 && robot->GetStatus() != RobotStatus::kDestroyed) {
            robot->SetStatus(RobotStatus::kDestroyed);
            OutputDestroyed(team, id);  // 输出击毁信息
            destroyed_robot_list_.push_back(robot);  // 移到击毁列表
            // 从正常列表中移除
            normal_robot_list_.erase(
                std::remove(normal_robot_list_.begin(), normal_robot_list_.end(), robot),
                normal_robot_list_.end());
        }
    }

    // 增加热量命令处理
    void IncreaseHeat(uint16_t team, uint16_t id, uint16_t heat) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;  // 机器人不存在
        }
        // 只有步兵可以增加热量
        if (robot->GetType() == RobotType::kInfantry) {
            robot->AddHeat(heat);
        }
    }

    // 升级机器人命令处理
    void UpgradeRobot(uint16_t team, uint16_t id, uint16_t target_level) {
        auto robot = FindRobot(team, id, normal_robot_list_);
        if (!robot) {
            return;  // 机器人不存在
        }
        // 只有步兵可以升级
        if (robot->GetType() != RobotType::kInfantry) {
            return;
        }
        // 检查并执行升级
        if (robot->CanUpgrade(target_level)) {
            robot->Upgrade(target_level);
        }
    }

    // 输出被击毁信息
    void OutputDestroyed(uint16_t team, uint16_t id) {
        std::cout << "D " << team << " " << id << std::endl;
    }
};

// 主函数
int main() {
    RobotManage manager;  // 创建机器人管理器
    manager.Run();        // 运行机器人管理系统
    return 0;
}
