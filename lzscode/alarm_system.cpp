#include "alarm_system.hpp"
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

float simulate_distance() {
    return static_cast<float>(rand() % 150 + 10);  // 10 ~ 160 cm
}

bool is_too_close(float distance, float threshold) {
    return distance < threshold;
}

void trigger_alarm(float distance) {
    std::cout << "[⚠️ 报警] 展品距离过近：" << distance << " cm！" << std::endl;
    system("aplay alarm.wav");  // 播放声音（确保 alarm.wav 文件在当前目录）
}

void log_event(float distance, const std::string& log_file) {
    std::ofstream file(log_file, std::ios::app);
    time_t now = time(0);
    char* dt = ctime(&now);  // 格式化时间字符串
    file << "ALARM | 距离: " << distance << " cm | 时间: " << dt;
    file.close();
}
