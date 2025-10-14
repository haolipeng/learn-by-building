# 基于二维数组的有限状态机

## 📖 简介

这是一个基于状态转换表（二维数组）实现的有限状态机，特别适合状态和事件都相对固定的场景，如网络协议状态机。

## 🎯 核心特点

### 💡 设计思想
- **状态转换表**: 使用二维数组 `transitionTable[状态][事件] = 新状态`
- **动作表**: 使用二维数组 `actionTable[状态][事件] = 动作索引`
- **索引映射**: 状态和事件使用整数索引，通过数组映射到名称

### ⚡ 性能优势
- **O(1) 查找**: 状态转换查找时间复杂度为常数
- **内存紧凑**: 连续的数组存储，缓存友好
- **高效执行**: 适合高频状态转换场景

### 🎯 适用场景
- **网络协议**: TCP/UDP状态机
- **编译器**: 词法分析器、语法分析器
- **游戏AI**: NPC状态控制
- **嵌入式系统**: 设备控制器

## 🚀 快速开始

### 运行演示

```bash
cd array_base_fsm
go run .
```

### 运行测试

```bash
go test -v
```

## 🔧 基本用法

### 创建状态机

```go
// 定义状态和事件名称
stateNames := []string{"锁定", "解锁", "开启"}
eventNames := []string{"输入密码", "推门", "关门", "锁门"}

// 创建状态机，初始状态为0(锁定)
fsm := NewTableBasedFSM(stateNames, eventNames, 0)
```

### 添加状态转换规则

```go
// 添加动作函数
action := fsm.AddAction(func(from, to, event int, data interface{}) error {
    fmt.Printf("状态转换: %s -> %s\n", 
        fsm.GetStateName(from), fsm.GetStateName(to))
    return nil
})

// 添加转换规则: 状态0 + 事件0 -> 状态1，执行动作action
fsm.AddTransition(0, 0, 1, action)
```

### 触发状态转换

```go
// 触发事件0
err := fsm.Trigger(0, nil)
if err != nil {
    fmt.Printf("转换失败: %v\n", err)
}

fmt.Printf("当前状态: %s\n", fsm.CurrentStateName())
```

## 📊 API 参考

### 核心类型

```go
// 状态机结构
type TableBasedFSM struct {
    transitionTable [][]int     // 状态转换表
    actionTable     [][]int     // 动作表
    actions         []TableAction // 动作函数数组
    stateNames      []string    // 状态名称
    eventNames      []string    // 事件名称
    currentState    int         // 当前状态索引
    mutex           sync.RWMutex // 并发安全锁
}

// 动作函数类型
type TableAction func(fromState, toState, event int, data interface{}) error
```

### 主要方法

#### `NewTableBasedFSM(stateNames, eventNames []string, initialState int) *TableBasedFSM`
创建新的状态机

**参数:**
- `stateNames`: 状态名称数组
- `eventNames`: 事件名称数组
- `initialState`: 初始状态索引

#### `AddAction(action TableAction) int`
添加动作函数

**返回:** 动作索引

#### `AddTransition(fromState, event, toState int, actionIndex int) error`
添加状态转换规则

**参数:**
- `fromState`: 源状态索引
- `event`: 事件索引
- `toState`: 目标状态索引
- `actionIndex`: 动作索引（可为 INVALID_ACTION）

#### `Trigger(event int, data interface{}) error`
触发状态转换

#### `CurrentState() int`
获取当前状态索引

#### `CurrentStateName() string`
获取当前状态名称

#### `GetAvailableEvents() []int`
获取当前状态下可用的事件索引

#### `Reset(state int) error`
重置到指定状态

#### `PrintTransitionTable()`
打印状态转换表（用于调试）

## 💡 示例

### TCP协议状态机

```go
// TCP状态常量
const (
    TCP_CLOSED      = 0
    TCP_LISTEN      = 1
    TCP_SYN_SENT    = 2
    TCP_ESTABLISHED = 4
    // ...
)

// TCP事件常量
const (
    TCP_OPEN    = 0
    TCP_SYN     = 2
    TCP_SYN_ACK = 3
    // ...
)

// 创建TCP状态机
tcpFSM := CreateTCPStateMachine()

// 模拟TCP三次握手
tcpFSM.Trigger(TCP_OPEN, nil)    // CLOSED -> SYN_SENT
tcpFSM.Trigger(TCP_SYN_ACK, nil) // SYN_SENT -> ESTABLISHED
```

### 门控系统

```go
stateNames := []string{"锁定", "解锁", "开启"}
eventNames := []string{"输入密码", "推门", "关门", "锁门"}

doorFSM := NewTableBasedFSM(stateNames, eventNames, 0)

// 添加转换规则
doorFSM.AddTransition(0, 0, 1, action) // 锁定 + 输入密码 -> 解锁
doorFSM.AddTransition(1, 1, 2, action) // 解锁 + 推门 -> 开启
doorFSM.AddTransition(2, 2, 1, action) // 开启 + 关门 -> 解锁
```

## 📈 性能特性

### 时间复杂度
- **状态转换查找**: O(1)
- **事件验证**: O(1)
- **状态名称获取**: O(1)

### 空间复杂度
- **转换表**: O(状态数 × 事件数)
- **动作表**: O(状态数 × 事件数)
- **名称映射**: O(状态数 + 事件数)

### 性能测试结果
```
BenchmarkTableBasedFSM-8    50000000    25.4 ns/op    0 B/op    0 allocs/op
```

## ⚖️ vs 基于Map的状态机

| 特性 | 基于表的状态机 | 基于Map的状态机 |
|------|----------------|-----------------|
| 查找速度 | O(1) | O(log n) |
| 内存使用 | 预分配，紧凑 | 按需分配 |
| 状态添加 | 编译时固定 | 运行时动态 |
| 类型安全 | 整数索引 | 字符串类型 |
| 适用场景 | 固定协议 | 业务逻辑 |

## 🎯 最佳实践

### ✅ 推荐用法
- 状态和事件数量相对固定
- 对性能要求较高
- 需要频繁状态转换
- 协议实现场景

### ❌ 不推荐用法
- 状态需要动态添加
- 业务逻辑经常变更
- 状态数量过大（>1000）
- 稀疏状态转换矩阵

## 🔧 调试技巧

### 打印状态转换表
```go
fsm.PrintTransitionTable()
```

### 检查当前可用事件
```go
events := fsm.GetAvailableEventNames()
fmt.Printf("可用事件: %v\n", events)
```

### 状态转换日志
```go
action := fsm.AddAction(func(from, to, event int, data interface{}) error {
    log.Printf("状态转换: %s -> %s (事件: %s)", 
        fsm.GetStateName(from), fsm.GetStateName(to), fsm.GetEventName(event))
    return nil
})
```

## 📚 学习资源

- [有限状态机原理](https://zh.wikipedia.org/wiki/有限状态机)
- [TCP协议状态机](https://tools.ietf.org/html/rfc793)
- [编译原理中的状态机](https://en.wikipedia.org/wiki/Finite-state_machine)

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

MIT License