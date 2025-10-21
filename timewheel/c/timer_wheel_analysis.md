# 时间轮（Timer Wheel）实现源码分析

## 概述

本文档详细分析基于 C 语言实现的时间轮（Timer Wheel）定时器系统。时间轮是一种高效的定时器管理数据结构，广泛应用于操作系统内核、网络协议栈和高性能服务器中。

## 目录

- [1. 核心概念](#1-核心概念)
- [2. 数据结构设计](#2-数据结构设计)
- [3. 核心算法实现](#3-核心算法实现)
- [4. API 接口说明](#4-api-接口说明)
- [5. 使用示例](#5-使用示例)
- [6. 性能分析](#6-性能分析)
- [7. 调试支持](#7-调试支持)

---

## 1. 核心概念

### 1.1 什么是时间轮？

时间轮是一种用于管理大量定时器的高效数据结构，其核心思想类似于钟表：

- **槽位（Slots）**：时间轮被划分为固定数量的槽位，每个槽位代表一个时间单位
- **循环推进**：时间指针随着时间推进，循环遍历各个槽位
- **哈希分布**：定时器根据过期时间分散到不同槽位中

### 1.2 时间复杂度

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| 添加定时器 | O(1) | 直接插入到对应槽位的链表尾部 |
| 删除定时器 | O(1) | 从链表中删除节点 |
| 刷新定时器 | O(1) | 删除后重新插入 |
| 推进时间轮 | O(N) | N 为过期定时器数量，非槽位数 |

### 1.3 设计参数

```c
#define MAX_TIMER_SLOTS 3600  // 时间轮槽位数量：3600
```

- **槽位数量**：3600 个，如果每个槽位代表 1 秒，则支持 1 小时范围内的定时器
- **最大超时**：`MAX_TIMER_SLOTS - 1`（3599），超过会被截断为 0
- **时间单位**：由使用者定义（秒、毫秒、时钟滴答等）

---

## 2. 数据结构设计

### 2.1 时间轮结构体

```c
typedef struct timer_wheel_ {
    struct cds_list_head slots[MAX_TIMER_SLOTS];  // 槽位数组
    uint32_t count;                                 // 活跃定时器总数
    uint32_t current;                               // 当前时间位置
} timer_wheel_t;
```

**字段说明：**

- `slots[]`：槽位数组，每个槽位是一个双向链表头节点（使用 URCU 库）
- `count`：当前活跃的定时器总数，用于统计和调试
- `current`：时间轮的当前时间位置，表示当前所在的时间点

### 2.2 定时器条目结构体

```c
typedef struct timer_entry_ {
    struct cds_list_head link;          // 链表节点
    timer_wheel_expire_fct callback;    // 过期回调函数
    uint16_t expire_slot;               // 过期槽位索引
    uint16_t timeout;                   // 超时值
#ifdef DEBUG_TIMER_WHEEL
    debug_entry_t history[16];          // 调试历史记录
    int debugs;                         // 调试记录计数
#endif
} timer_entry_t;
```

**字段说明：**

- `link`：用于链接到槽位链表的节点
- `callback`：定时器过期时调用的回调函数，NULL 表示定时器未激活
- `expire_slot`：定时器将要过期的槽位索引（0 ~ MAX_TIMER_SLOTS-1）
- `timeout`：定时器的超时值（时间单位数）
- `history[]` 和 `debugs`：调试模式下的操作历史记录

### 2.3 回调函数类型

```c
typedef void (*timer_wheel_expire_fct)(struct timer_entry_ *n);
```

定时器过期时调用的回调函数类型，参数为过期的定时器条目指针。

---

## 3. 核心算法实现

### 3.1 初始化（timer_wheel_init）

```c
void timer_wheel_init(timer_wheel_t *w)
{
    int i;
    for (i = 0; i < MAX_TIMER_SLOTS; i++) {
        CDS_INIT_LIST_HEAD(&w->slots[i]);
    }
    w->current = w->count = 0;
}
```

**功能**：初始化时间轮的所有槽位为空链表，重置计数器和时间位置。

**步骤**：
1. 遍历所有槽位，初始化每个槽位的链表头
2. 将当前时间和定时器计数重置为 0

### 3.2 启动时间轮（timer_wheel_start）

```c
void timer_wheel_start(timer_wheel_t *w, uint32_t now)
{
    w->current = now;
}
```

**功能**：设置时间轮的起始时间。

**使用场景**：通常在初始化后调用一次，设置时间基准点。

### 3.3 时间推进与定时器触发（timer_wheel_roll）

这是时间轮的核心函数，负责推进时间并触发过期定时器。

```c
uint32_t timer_wheel_roll(timer_wheel_t *w, uint32_t now)
{
    if (now < w->current) {
        return 0;  // 时间倒退，不处理
    }

    uint32_t cnt = 0;
    uint32_t s, m = min(now, w->current + MAX_TIMER_SLOTS);

    for (s = w->current; s < m; s++) {
        struct cds_list_head *head = &w->slots[s % MAX_TIMER_SLOTS];

        // 逐个处理槽位中的定时器
        while (!cds_list_empty(head)) {
            timer_entry_t *itr = cds_list_first_entry(head, timer_entry_t, link);
            timer_wheel_expire_fct fn = itr->callback;
            timer_wheel_entry_remove(w, itr);
            fn(itr);
            cnt++;
        }
    }

    w->current = now;
    return cnt;
}
```

**算法步骤**：

1. **时间校验**：如果 `now < current`，说明时间倒退，直接返回 0
2. **范围限制**：计算推进范围 `m = min(now, current + MAX_TIMER_SLOTS)`
   - 防止一次推进过多槽位导致性能问题
   - 最多推进 `MAX_TIMER_SLOTS` 个槽位
3. **遍历槽位**：从 `current` 到 `m-1`，遍历每个经过的槽位
4. **处理过期定时器**：
   - 从链表头部逐个取出定时器
   - 先移除定时器，再调用回调函数
   - 使用 `while` 循环而非迭代器，因为回调函数可能修改链表
5. **更新时间**：将 `current` 更新为 `now`
6. **返回值**：返回触发的定时器数量

**关键设计点**：

- **非安全遍历**：使用 `while` + 取头节点的方式，而非 `cds_list_for_each_entry_safe()`
  - 原因：回调函数中可能再次插入、删除定时器，导致迭代器失效
- **先删后调**：先从链表移除，再调用回调，避免回调中的操作影响遍历

### 3.4 定时器条目操作

#### 3.4.1 初始化定时器条目

```c
void timer_wheel_entry_init(timer_entry_t *n)
{
    CDS_INIT_LIST_HEAD(&n->link);
    n->expire_slot = (uint16_t)(-1);
    n->callback = NULL;
}
```

**功能**：将定时器条目初始化为未激活状态。

#### 3.4.2 插入定时器

```c
void timer_wheel_entry_insert(timer_wheel_t *w, timer_entry_t *n, uint32_t now)
{
    uint32_t expire_at = now + n->timeout;
    n->expire_slot = expire_at % MAX_TIMER_SLOTS;
    cds_list_add_tail(&n->link, &w->slots[n->expire_slot]);
    w->count++;
}
```

**槽位计算公式**：
```
expire_slot = (now + timeout) % MAX_TIMER_SLOTS
```

**示例**：
- 当前时间 `now = 100`
- 超时时间 `timeout = 50`
- 过期时间 `expire_at = 150`
- 槽位索引 `expire_slot = 150 % 3600 = 150`

**注意事项**：
- 定时器被插入到槽位链表的**尾部**
- 同一槽位可能有多个定时器（哈希冲突）

#### 3.4.3 删除定时器

```c
void timer_wheel_entry_remove(timer_wheel_t *w, timer_entry_t *n)
{
    cds_list_del(&n->link);
    w->count--;
    n->callback = NULL;  // 标记为非激活状态
}
```

**功能**：从时间轮中移除定时器，取消其过期回调。

**状态标记**：将 `callback` 设为 NULL 表示定时器未激活。

#### 3.4.4 刷新定时器

```c
void timer_wheel_entry_refresh(timer_wheel_t *w, timer_entry_t *n, uint32_t now)
{
    timer_wheel_expire_fct fn = n->callback;
    timer_wheel_entry_remove(w, n);
    n->callback = fn;
    timer_wheel_entry_insert(w, n, now);
}
```

**功能**：重置定时器的超时时间，从当前时刻重新计时。

**使用场景**：
- TCP keepalive：收到数据包时刷新连接超时定时器
- 会话管理：用户活动时刷新会话过期时间

#### 3.4.5 启动定时器

```c
void timer_wheel_entry_start(timer_wheel_t *w, timer_entry_t *n,
                             timer_wheel_expire_fct cb, uint16_t timeout, uint32_t now)
{
    if (unlikely(timeout >= MAX_TIMER_SLOTS)) {
        timeout = 0;  // 超时值过大，截断为 0
    }

    n->callback = cb;
    n->timeout = timeout;
    timer_wheel_entry_insert(w, n, now);
}
```

**功能**：设置回调函数和超时值，然后启动定时器。

**参数检查**：如果超时值 ≥ MAX_TIMER_SLOTS，会被截断为 0。

### 3.5 时间计算辅助函数

#### 3.5.1 计算过期时间（get_expire_at）

```c
static uint32_t get_expire_at(const timer_entry_t *n, uint32_t now)
{
    uint16_t slot = now % MAX_TIMER_SLOTS;
    if (n->callback == NULL || n->expire_slot >= slot) {
        return now + n->expire_slot - slot;
    } else {
        return now + n->expire_slot + MAX_TIMER_SLOTS - slot;
    }
}
```

**功能**：计算定时器的绝对过期时间。

**算法逻辑**：
1. 计算当前时间对应的槽位 `slot = now % MAX_TIMER_SLOTS`
2. 判断过期槽位与当前槽位的位置关系：
   - 如果 `expire_slot >= slot`：在同一轮内，过期时间 = `now + (expire_slot - slot)`
   - 如果 `expire_slot < slot`：在下一轮，过期时间 = `now + (expire_slot + MAX_TIMER_SLOTS - slot)`

**示例**：
```
MAX_TIMER_SLOTS = 3600
now = 3650, slot = 3650 % 3600 = 50
expire_slot = 100

过期时间 = 3650 + (100 - 50) = 3700
```

```
now = 3650, slot = 50
expire_slot = 30

过期时间 = 3650 + (30 + 3600 - 50) = 3650 + 3580 = 7230
```

#### 3.5.2 获取已运行时间（timer_wheel_entry_get_idle）

```c
uint16_t timer_wheel_entry_get_idle(const timer_entry_t *n, uint32_t now)
{
    return (uint16_t)(now - (get_expire_at(n, now) - n->timeout));
}
```

**功能**：计算定时器已经运行的时间。

**公式**：
```
idle = now - (expire_at - timeout)
     = now - start_time
```

#### 3.5.3 获取剩余时间（timer_wheel_entry_get_life）

```c
uint16_t timer_wheel_entry_get_life(const timer_entry_t *n, uint32_t now)
{
    return (uint16_t)(get_expire_at(n, now) - now);
}
```

**功能**：计算定时器还有多久过期。

**公式**：
```
life = expire_at - now
```

---

## 4. API 接口说明

### 4.1 时间轮管理 API

| 函数 | 功能 | 时间复杂度 |
|------|------|-----------|
| `timer_wheel_init(w)` | 初始化时间轮 | O(N)，N = MAX_TIMER_SLOTS |
| `timer_wheel_start(w, now)` | 设置起始时间 | O(1) |
| `timer_wheel_roll(w, now)` | 推进时间并触发过期定时器 | O(M)，M = 过期定时器数 |
| `timer_wheel_current(w)` | 获取当前时间 | O(1) |
| `timer_wheel_count(w)` | 获取活跃定时器数量 | O(1) |
| `timer_wheel_started(w)` | 判断时间轮是否已启动 | O(1) |

### 4.2 定时器条目 API

| 函数 | 功能 | 时间复杂度 |
|------|------|-----------|
| `timer_wheel_entry_init(n)` | 初始化定时器条目 | O(1) |
| `timer_wheel_entry_start(w, n, cb, timeout, now)` | 启动定时器 | O(1) |
| `timer_wheel_entry_insert(w, n, now)` | 插入定时器到时间轮 | O(1) |
| `timer_wheel_entry_remove(w, n)` | 移除定时器 | O(1) |
| `timer_wheel_entry_refresh(w, n, now)` | 刷新定时器 | O(1) |
| `timer_wheel_entry_is_active(n)` | 判断定时器是否激活 | O(1) |
| `timer_wheel_entry_get_idle(n, now)` | 获取已运行时间 | O(1) |
| `timer_wheel_entry_get_life(n, now)` | 获取剩余时间 | O(1) |
| `timer_wheel_entry_set_timeout(n, timeout)` | 设置超时值 | O(1) |
| `timer_wheel_entry_get_timeout(n)` | 获取超时值 | O(1) |
| `timer_wheel_entry_set_callback(n, cb)` | 设置回调函数 | O(1) |

---

## 5. 使用示例

### 5.1 基本使用流程

```c
#include "timer_wheel.h"

// 定时器过期回调函数
void on_timeout(timer_entry_t *entry)
{
    printf("Timer expired!\n");
    // 处理超时逻辑
}

int main()
{
    timer_wheel_t wheel;
    timer_entry_t timer1, timer2;
    uint32_t current_time = 0;

    // 1. 初始化时间轮
    timer_wheel_init(&wheel);
    timer_wheel_start(&wheel, current_time);

    // 2. 初始化定时器条目
    timer_wheel_entry_init(&timer1);
    timer_wheel_entry_init(&timer2);

    // 3. 启动定时器
    timer_wheel_entry_start(&wheel, &timer1, on_timeout, 10, current_time);
    timer_wheel_entry_start(&wheel, &timer2, on_timeout, 20, current_time);

    // 4. 推进时间
    for (int i = 0; i < 30; i++) {
        current_time++;
        uint32_t expired = timer_wheel_roll(&wheel, current_time);
        printf("Time %u: %u timers expired\n", current_time, expired);
    }

    return 0;
}
```

### 5.2 刷新定时器（Keepalive 示例）

```c
void connection_activity(timer_wheel_t *wheel, timer_entry_t *conn_timer, uint32_t now)
{
    // 收到数据包，刷新连接超时定时器
    if (timer_wheel_entry_is_active(conn_timer)) {
        timer_wheel_entry_refresh(wheel, conn_timer, now);
    }
}
```

### 5.3 取消定时器

```c
void cancel_timer(timer_wheel_t *wheel, timer_entry_t *timer)
{
    if (timer_wheel_entry_is_active(timer)) {
        timer_wheel_entry_remove(wheel, timer);
    }
}
```

---

## 6. 性能分析

### 6.1 时间复杂度总结

| 场景 | 时间复杂度 | 说明 |
|------|-----------|------|
| 添加 N 个定时器 | O(N) | 每个定时器 O(1) |
| 时间推进 T 个单位 | O(E) | E 为过期定时器数，与 T 无关 |
| 查询定时器状态 | O(1) | 直接访问字段 |

### 6.2 空间复杂度

- **时间轮**：`sizeof(cds_list_head) * MAX_TIMER_SLOTS ≈ 16 * 3600 = 57.6KB`
- **每个定时器**：`sizeof(timer_entry_t) ≈ 24 字节`（不含调试信息）

### 6.3 优势

1. **O(1) 插入/删除**：定时器操作不受总定时器数量影响
2. **批量触发高效**：时间推进只处理过期定时器，不遍历所有定时器
3. **内存局部性好**：槽位数组连续存储，缓存友好
4. **支持大量定时器**：可轻松管理数万到数百万定时器

### 6.4 劣势与限制

1. **超时范围限制**：最大超时值受 `MAX_TIMER_SLOTS` 限制
2. **内存开销**：即使没有定时器也需要预分配槽位数组
3. **时间精度**：受槽位数量和推进粒度限制
4. **非精确触发**：定时器在槽位对应时间点触发，可能有偏差

---

## 7. 调试支持

### 7.1 调试模式

编译时定义 `DEBUG_TIMER_WHEEL` 宏启用调试功能：

```c
#ifdef DEBUG_TIMER_WHEEL
typedef struct debug_entry_ {
    void *caller[4];      // 调用栈地址
    void *callback;       // 回调函数地址
    uint8_t act;          // 操作类型：INSERT(0) 或 REMOVE(1)
} debug_entry_t;
#endif
```

### 7.2 调试功能

1. **操作历史记录**：记录最近 16 次插入/删除操作
2. **双重插入检测**：断言捕获连续两次插入操作
3. **双重删除检测**：断言捕获连续两次删除操作
4. **调用栈追踪**：记录操作发生时的调用栈

### 7.3 调试日志示例

```c
#ifdef DEBUG_TIMER_WHEEL
// 在插入操作中记录调试信息
n->history[n->debugs].caller[0] = __builtin_return_address(0);
n->history[n->debugs].callback = n->callback;
n->history[n->debugs].act = INSERT;
n->debugs++;

// 检测双重插入
if (n->debugs >= 2 && n->history[n->debugs - 2].act == INSERT) {
    assert(0);  // 触发断言
}
#endif
```

---

## 8. 实现要点与最佳实践

### 8.1 线程安全性

**当前实现不是线程安全的**。如果在多线程环境中使用，需要：

1. 使用互斥锁保护时间轮操作
2. 或者确保单线程访问（如事件循环）
3. URCU 链表支持 RCU 并发，但需要额外的同步机制

### 8.2 回调函数注意事项

在定时器回调函数中可以：
- ✅ 启动新的定时器
- ✅ 删除其他定时器
- ✅ 刷新定时器

但要注意：
- ⚠️ 避免在回调中进行耗时操作，会延迟其他定时器触发
- ⚠️ 回调中修改当前定时器条目时要小心，因为已被移除

### 8.3 时间溢出处理

使用 32 位 `uint32_t` 表示时间，会在约 `2^32` 个时间单位后溢出：
- 如果时间单位为秒，约 136 年溢出
- 如果时间单位为毫秒，约 49.7 天溢出

代码未处理溢出情况，如果需要长期运行，建议：
1. 使用 64 位时间戳
2. 或实现溢出检测逻辑

### 8.4 槽位数量选择

槽位数量的选择需要权衡：

| 槽位数 | 优点 | 缺点 |
|--------|------|------|
| 较少（如 256） | 内存占用小 | 哈希冲突多，同一槽位定时器多 |
| 适中（如 3600） | 平衡性能和内存 | 适合大多数场景 |
| 较多（如 86400） | 冲突少，精度高 | 内存占用大，初始化慢 |

---

## 9. 应用场景

### 9.1 网络协议栈

- **TCP 超时重传**：每个 TCP 连接的重传定时器
- **TCP Keepalive**：检测连接存活性
- **ARP 缓存过期**：清理 ARP 表项
- **DNS 缓存过期**：DNS 记录 TTL 管理

### 9.2 Web 服务器

- **会话超时**：HTTP 会话过期管理
- **请求超时**：防止慢速客户端占用连接
- **连接池管理**：空闲连接回收

### 9.3 操作系统内核

- **进程调度**：时间片轮转
- **定时任务**：cron 作业调度
- **硬件看门狗**：系统健康检查

---

## 10. 与其他定时器方案对比

| 方案 | 插入 | 删除 | 触发 | 内存 | 适用场景 |
|------|------|------|------|------|----------|
| 时间轮 | O(1) | O(1) | O(E) | O(N) | 大量定时器，超时范围有限 |
| 最小堆 | O(logN) | O(logN) | O(E·logN) | O(N) | 定时器数量适中，超时范围广 |
| 红黑树 | O(logN) | O(logN) | O(E·logN) | O(N) | 需要有序遍历定时器 |
| 链表 | O(1) | O(1) | O(N) | O(N) | 定时器数量少 |

**E**: 过期定时器数量
**N**: 总定时器数量

---

## 11. 总结

本时间轮实现是一个**高性能、低延迟**的定时器管理系统，特别适合：

✅ **大规模定时器场景**（数千到数百万）
✅ **超时范围有限**（如 1 小时内）
✅ **高频插入/删除操作**（如网络连接管理）
✅ **实时性要求高**（如内核、网络协议栈）

通过合理配置槽位数量和时间单位，可以在性能、内存和精度之间取得良好平衡。

---

## 12. 参考资料

- **URCU 库文档**：[liburcu](https://liburcu.org/)
- **Linux 内核时间轮**：`kernel/time/timer.c`
- **论文**：*Hashed and Hierarchical Timing Wheels* by George Varghese and Tony Lauck

---

**文档版本**：1.0
**最后更新**：2025-10-21
**作者**：基于时间轮源码分析生成
