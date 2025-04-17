#include "alarm_system.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>

// 模拟生成随机距离（单位：cm）
float simulate_distance() {
    return static_cast<float>(rand() % 150 + 10);  // 生成 10~159 cm
}

// 判断是否小于报警阈值
bool is_too_close(float distance, float threshold) {
    return distance < threshold;
}

// 报警执行逻辑（声音 + 人声 + 图形弹窗）
void trigger_alarm(float distance) {
    std::cout << "[⚠️ 报警] 展品距离过近：" << distance << " cm！" << std::endl;

    // 播放警报音效
    system("aplay alarm.wav &");

    // 播放语音播报
    system("espeak '警报，展品距离过近！！！' &");

    // 构建图形弹窗的文本（大字体、红色、加粗）
    std::string text = "<span font='24' weight='bold' foreground='red'>警报，展品距离过近！！！</span>";

    // 弹出多个图形弹窗（3个）
    for (int i = 0; i < 3; ++i) {
        std::thread([text]() {
            std::string cmd = "yad --text=\"" + text +
                              "\" --title='展品警报' --window-icon=dialog-error "
                              "--width=350 --height=120 --timeout=5 --center --no-buttons --on-top";
            system(cmd.c_str());
        }).detach();

        // 弹窗之间稍微间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// 将报警事件记录到日志
void log_event(float distance, const std::string& log_file) {
    std::ofstream file(log_file, std::ios::app);
    time_t now = time(0);
    char* dt = ctime(&now);  // 当前时间字符串
    file << "ALARM | 距离: " << distance << " cm | 时间: " << dt;
    file.close();
}

