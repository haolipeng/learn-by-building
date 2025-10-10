#include "timeWheel.h"
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "=== 时间轮定时器演示程序 ===" << std::endl;
    
    // 创建时间轮，设置空闲时间为3秒
    CTimeWheelBoost timeWheel(3, NULL);
    
    // 创建一些测试会话
    Sessionkey session1("192.168.1.1", "192.168.1.2", 8080, 9090);
    Sessionkey session2("192.168.1.3", "192.168.1.4", 8081, 9091);
    
    std::cout << "添加会话1..." << std::endl;
    timeWheel.AddElement(session1);
    
    std::cout << "添加会话2..." << std::endl;
    timeWheel.AddElement(session2);
    
    // 打印当前状态
    std::cout << "当前时间轮状态:" << std::endl;
    timeWheel.dumpSessionKeyBuckets();
    
    // 等待一段时间观察
    std::cout << "等待2秒..." << std::endl;
    sleep(2);
    
    std::cout << "2秒后的状态:" << std::endl;
    timeWheel.dumpSessionKeyBuckets();
    
    // 等待超时
    std::cout << "等待超时（3秒后元素应该被清理）..." << std::endl;
    sleep(3);
    
    std::cout << "超时后的状态:" << std::endl;
    timeWheel.dumpSessionKeyBuckets();
    
    std::cout << "程序结束" << std::endl;
    return 0;
}