#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>  // sleep
#include "alarm_system.hpp"

const float THRESHOLD = 50.0f;                      // 报警距离阈值
const std::string LOG_FILE = "alarm_log.txt";       // 日志文件名

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    while (true) {
        float distance = simulate_distance();
        std::cout << "[Sensor] 当前距离: " << distance << " cm" << std::endl;

        if (is_too_close(distance, THRESHOLD)) {
            trigger_alarm(distance);
            log_event(distance, LOG_FILE);
        } else {
            std::cout << "[✅ 安全] 当前距离：" << distance << " cm\n" << std::endl;
        }

        sleep(2);  // 每两秒检测一次
    }

    return 0;
}
