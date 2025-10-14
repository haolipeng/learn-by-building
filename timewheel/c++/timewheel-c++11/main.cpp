#include "timeWheel.h"
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "=== TCP会话时间轮演示程序 ===" << std::endl;

    // 创建时间轮，设置会话超时时间为5秒
    CTimeWheel timeWheel(5, NULL);

    // 创建测试会话（模拟TCP连接）
    Sessionkey session1("192.168.1.100", "10.0.0.1", 80, 54321, 6);  // Web服务器连接
    Sessionkey session2("192.168.1.100", "10.0.0.2", 443, 54322, 6); // HTTPS连接

    std::cout << "\n[时刻 0s] 添加会话1 (HTTP)..." << std::endl;
    timeWheel.AddElement(session1);

    std::cout << "[时刻 0s] 添加会话2 (HTTPS)..." << std::endl;
    timeWheel.AddElement(session2);

    sleep(1);

    // 模拟接收到上行数据（客户端发送数据）
    std::cout << "\n[时刻 1s] 会话1接收上行数据 1500 字节..." << std::endl;
    timeWheel.UpdateSession(session1, true, 1500, 1);

    std::cout << "[时刻 1s] 会话2接收上行数据 500 字节..." << std::endl;
    timeWheel.UpdateSession(session2, true, 500, 1);

    sleep(1);

    // 模拟接收到下行数据（服务器响应）
    std::cout << "\n[时刻 2s] 会话1接收下行数据 4096 字节..." << std::endl;
    timeWheel.UpdateSession(session1, false, 4096, 3);

    // 查询会话统计
    SessionStats stats1;
    if(timeWheel.GetSessionStats(session1, stats1)) {
        std::cout << "会话1当前统计: 上行=" << stats1.upBytes << "B/"
                  << stats1.upPackets << "pkts, 下行=" << stats1.downBytes
                  << "B/" << stats1.downPackets << "pkts" << std::endl;
    }

    sleep(1);

    // 会话1继续有数据传输
    std::cout << "\n[时刻 3s] 会话1继续传输上行数据 800 字节..." << std::endl;
    timeWheel.UpdateSession(session1, true, 800, 1);

    std::cout << "[时刻 3s] 会话1接收下行数据 2048 字节..." << std::endl;
    timeWheel.UpdateSession(session1, false, 2048, 2);

    // 会话2没有数据传输，将会超时

    sleep(3);

    std::cout << "\n[时刻 6s] 会话2应该已超时（最后活动在1s，超时时间5s）" << std::endl;
    std::cout << "会话1应该仍然活跃（最后活动在3s，还未达到超时时间）" << std::endl;

    // 再次查询会话1统计
    if(timeWheel.GetSessionStats(session1, stats1)) {
        std::cout << "会话1最终统计: 上行=" << stats1.upBytes << "B/"
                  << stats1.upPackets << "pkts, 下行=" << stats1.downBytes
                  << "B/" << stats1.downPackets << "pkts" << std::endl;
    }

    // 打印时间轮状态
    std::cout << "\n当前时间轮状态:" << std::endl;
    timeWheel.dumpSessionKeyBuckets();

    sleep(3);

    std::cout << "\n[时刻 9s] 会话1也应该超时了（最后活动在3s，已超过5秒）" << std::endl;

    std::cout << "\n=== 演示结束 ===" << std::endl;
    std::cout << "总共超时会话数: " << timeoutNum << std::endl;

    return 0;
}