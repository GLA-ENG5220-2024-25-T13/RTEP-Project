#ifndef ALARM_SYSTEM_HPP
#define ALARM_SYSTEM_HPP

#include <string>

// 模拟距离传感器
float simulate_distance();

// 判断是否小于安全阈值
bool is_too_close(float distance, float threshold);

// 报警触发（声音 + 输出）
void trigger_alarm(float distance);

// 写入报警日志
void log_event(float distance, const std::string& log_file);

#endif
