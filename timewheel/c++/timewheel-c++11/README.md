# TCP会话时间轮组件

## 功能概述

这是一个支持TCP会话管理的时间轮组件，能够：
- 管理TCP会话的生命周期（基于五元组）
- 自动超时清理不活跃的会话
- 统计每个会话的上下行字节数和数据包数
- 接收到数据后自动刷新会话的生命周期
- 线程安全（使用互斥锁保护）

## 核心改进

### 1. TCP会话统计结构
```cpp
struct SessionStats {
    uint64_t upBytes;      // 上行字节数
    uint64_t downBytes;    // 下行字节数
    uint64_t upPackets;    // 上行数据包数
    uint64_t downPackets;  // 下行数据包数
};
```

### 2. 五元组会话Key
```cpp
class Sessionkey {
    std::string dstIp;
    std::string srcIp;
    int dstPort;
    int srcPort;
    uint8_t protocol;      // 协议类型（TCP=6）
    SessionStats stats;    // 会话统计信息
};
```

### 3. 性能优化
- **避免全遍历**：Entry中记录bucket索引，移除元素时直接定位，不需要遍历所有bucket
- **线程安全**：添加`std::mutex`保护共享数据结构
- **双向查找**：支持正向和反向五元组查找（因为TCP连接是双向的）

## API说明

### 创建时间轮
```cpp
CTimeWheel timeWheel(int idleSeconds, void* timeoutQueue);
// idleSeconds: 会话超时时间（秒）
// timeoutQueue: 超时队列指针（可选）
```

### 添加新会话
```cpp
Sessionkey session("192.168.1.1", "10.0.0.1", 80, 54321, 6);
bool success = timeWheel.AddElement(session);
// 返回true表示成功添加，false表示会话已存在
```

### 更新会话（核心方法）
```cpp
bool UpdateSession(const Sessionkey& key,
                   bool isUplink,          // true=上行, false=下行
                   uint64_t bytes,         // 字节数
                   uint64_t packets = 1);  // 数据包数

// 示例：接收到客户端上行数据
timeWheel.UpdateSession(session, true, 1500, 1);

// 示例：接收到服务器下行数据
timeWheel.UpdateSession(session, false, 4096, 3);
```

**功能**：
1. 更新会话的统计信息（累加字节数和包数）
2. 刷新会话的生命周期（移动到最新的bucket）
3. 如果会话不存在，会自动创建

### 查询会话统计
```cpp
SessionStats stats;
if(timeWheel.GetSessionStats(session, stats)) {
    std::cout << "上行: " << stats.upBytes << " 字节, "
              << stats.upPackets << " 包" << std::endl;
    std::cout << "下行: " << stats.downBytes << " 字节, "
              << stats.downPackets << " 包" << std::endl;
}
```

## 使用示例

### 基本用法
```cpp
#include "timeWheel.h"

int main() {
    // 1. 创建时间轮（超时时间5秒）
    CTimeWheel timeWheel(5, NULL);

    // 2. 创建会话
    Sessionkey session("192.168.1.100", "10.0.0.1", 80, 54321, 6);

    // 3. 添加会话
    timeWheel.AddElement(session);

    // 4. 接收到数据后更新
    timeWheel.UpdateSession(session, true, 1500);   // 上行1500字节
    timeWheel.UpdateSession(session, false, 4096);  // 下行4096字节

    // 5. 查询统计
    SessionStats stats;
    if(timeWheel.GetSessionStats(session, stats)) {
        std::cout << "上行: " << stats.upBytes << "B" << std::endl;
        std::cout << "下行: " << stats.downBytes << "B" << std::endl;
    }

    return 0;
}
```

### 实际应用场景：数据包处理
```cpp
// 在数据包处理函数中
void on_packet_received(const char* src_ip, const char* dst_ip,
                       uint16_t src_port, uint16_t dst_port,
                       const uint8_t* data, size_t len,
                       bool is_outgoing) {
    // 创建会话key
    Sessionkey session(dst_ip, src_ip, dst_port, src_port, 6);

    // 更新会话：刷新生命周期并累加统计
    timeWheel.UpdateSession(session, is_outgoing, len, 1);
}
```

## 工作原理

### 时间轮结构
```
时间轴: t0 -> t1 -> t2 -> t3 -> t4 -> t5 (oldest)
         |     |     |     |     |     |
Bucket: [0]   [1]   [2]   [3]   [4]   [5]
         ↑ 最新                    ↑ 最旧
```

### 生命周期刷新
- 每当接收到数据（调用`UpdateSession`），会话会被移动到最新的bucket
- 时间轮每秒tick一次，推出最旧的bucket，旧bucket中的Entry被析构
- Entry析构时会从keyMap中移除并输出统计信息

### 线程安全
- `AddElement`、`UpdateSession`、`GetSessionStats`都使用互斥锁保护
- `tickStepRun`在后台线程中运行，添加新bucket时也加锁

## 编译和运行

### 编译
```bash
g++ -std=c++11 -pthread timeWheel.cpp main.cpp -o timewheel_demo
```

### 运行
```bash
./timewheel_demo
```

## 输出示例
```
=== TCP会话时间轮演示程序 ===

[时刻 0s] 添加会话1 (HTTP)...
[时刻 0s] 添加会话2 (HTTPS)...

[时刻 1s] 会话1接收上行数据 1500 字节...
会话1当前统计: 上行=1500B/1pkts, 下行=0B/0pkts

[时刻 2s] 会话1接收下行数据 4096 字节...
会话1当前统计: 上行=1500B/1pkts, 下行=4096B/3pkts

[时刻 6s] 会话2应该已超时...
use_count is 1 element timeout! index is 0 Stats: up=500B/1pkts down=0B/0pkts
```

## 注意事项

1. **双向查找**：会话支持正反向五元组查找，因为TCP连接是双向的
2. **自动创建**：`UpdateSession`如果发现会话不存在会自动创建
3. **线程安全**：所有公共方法都是线程安全的
4. **性能优化**：避免了遍历所有bucket的开销，使用bucket索引直接定位

## 未来改进建议

1. 使用`std::chrono`替代`sleep`提高精度
2. 支持可配置的超时回调函数
3. 添加会话状态（建立、活跃、关闭等）
4. 支持持久化统计数据
5. 添加内存池优化Entry分配
