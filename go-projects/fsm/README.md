# Go 有限状态机 (Finite State Machine)

## 📖 简介

这是一个专为初学者设计的Go语言有限状态机实现。项目专注于核心概念，去除了复杂的回调机制，让学习者能够更好地理解状态机的本质原理。

## 🎯 核心概念

### 基本要素

1. **状态 (State)**: 系统在某个时刻的情况
2. **事件 (Event)**: 触发状态改变的条件  
3. **转换 (Transition)**: 从一个状态到另一个状态的规则
4. **动作 (Action)**: 状态转换时可选的操作

### 工作原理

1. 状态机始终处于某个确定的状态
2. 接收到事件时，检查是否有对应的转换规则
3. 如果有规则，执行转换动作并切换到新状态
4. 如果无规则，保持当前状态不变

### 核心特性

- **确定性**: 相同状态+相同事件 = 相同结果
- **有限性**: 状态数量是有限的
- **单一性**: 同时只能处于一个状态

## 🚀 快速开始

### 运行项目

```bash
cd go-projects/fsm
# 运行演示程序
go run cmd/demo/main.go

# 或者运行测试查看示例
go test -v
```

### 基本用法

```go
package main

import "fsm"

func main() {
    // 1. 定义状态和事件
    const (
        OFF fsm.State = "关闭"
        ON  fsm.State = "开启"
    )

    const (
        TURN_ON  fsm.Event = "打开"
        TURN_OFF fsm.Event = "关闭"
    )

    // 2. 创建状态机
    machine := fsm.NewFSM(OFF)

    // 3. 添加转换规则
    machine.AddTransition(OFF, TURN_ON, ON, nil)
    machine.AddTransition(ON, TURN_OFF, OFF, nil)

    // 4. 触发状态转换
    err := machine.Trigger(TURN_ON, nil)
    fmt.Println(machine.Current()) // 输出: 开启
}
```

## 📁 项目结构

```
go-projects/fsm/
├── fsm.go              # 核心状态机实现
├── fsm_test.go         # 测试和示例代码
├── cmd/demo/main.go    # 演示程序
├── go.mod              # Go模块配置
└── README.md           # 项目文档
```

## 🔧 API 参考

### 核心类型

```go
// 状态类型
type State string

// 事件类型  
type Event string

// 动作函数类型
type Action func(from, to State, data interface{}) error

// 状态机结构
type FSM struct {
    current     State
    transitions map[State]map[Event]Transition
    mutex       sync.RWMutex
}
```

### 主要方法

#### `NewFSM(initial State) *FSM`
创建新的状态机实例

**参数:**
- `initial`: 初始状态

**返回:**
- 状态机实例

#### `AddTransition(from State, event Event, to State, action Action)`
添加状态转换规则

**参数:**
- `from`: 源状态
- `event`: 触发事件
- `to`: 目标状态
- `action`: 转换动作 (可为nil)

#### `Trigger(event Event, data interface{}) error`
触发状态转换

**参数:**
- `event`: 要触发的事件
- `data`: 传递给动作函数的数据

**返回:**
- 错误信息 (如果转换失败)

#### `Current() State`
获取当前状态

**返回:**
- 当前状态

#### `GetAvailableEvents() []Event`
获取当前状态下可用的事件列表

**返回:**
- 可用事件列表

#### `Reset(state State)`
重置状态机到指定状态

**参数:**
- `state`: 要重置到的状态

## 💡 示例场景

### 1. 简单开关

```go
const (
    OFF SimpleState = "关闭"
    ON  SimpleState = "开启"
)

const (
    TURN_ON  SimpleEvent = "打开"
    TURN_OFF SimpleEvent = "关闭"
)

fsm := NewSimpleFSM(OFF)
fsm.AddTransition(OFF, TURN_ON, ON, nil)
fsm.AddTransition(ON, TURN_OFF, OFF, nil)
```

### 2. 订单状态管理

```go
const (
    CREATED   SimpleState = "已创建"
    PAID      SimpleState = "已支付"
    SHIPPED   SimpleState = "已发货"
    DELIVERED SimpleState = "已送达"
    CANCELLED SimpleState = "已取消"
)

const (
    PAY     SimpleEvent = "支付"
    SHIP    SimpleEvent = "发货"
    DELIVER SimpleEvent = "送达"
    CANCEL  SimpleEvent = "取消"
)

orderFSM := NewSimpleFSM(CREATED)
orderFSM.AddTransition(CREATED, PAY, PAID, func(from, to SimpleState, data interface{}) error {
    fmt.Printf("订单支付: %s -> %s\n", from, to)
    return nil
})
// ... 更多转换规则
```

### 3. 红绿灯系统

```go
const (
    RED    SimpleState = "红灯"
    YELLOW SimpleState = "黄灯"
    GREEN  SimpleState = "绿灯"
)

const (
    NEXT SimpleEvent = "下一个"
)

trafficLight := NewSimpleFSM(RED)
trafficLight.AddTransition(RED, NEXT, GREEN, nil)
trafficLight.AddTransition(GREEN, NEXT, YELLOW, nil)
trafficLight.AddTransition(YELLOW, NEXT, RED, nil)
```

## 🎓 适用场景

- **工作流控制**: 审批流程、任务状态管理
- **游戏开发**: 游戏状态管理 (开始/暂停/结束)
- **网络协议**: 连接状态管理 (连接/传输/断开)
- **用户界面**: UI状态控制 (登录/浏览/编辑)
- **设备控制**: 设备状态管理
- **业务流程**: 订单处理、支付流程

## 🔍 设计指南

### 如何设计状态机

1. **识别所有可能的状态**
   - 列出系统的所有关键状态
   - 确保状态是互斥的

2. **定义触发转换的事件**
   - 明确什么条件会触发状态改变
   - 事件应该是具体和明确的

3. **确定状态转换规则**
   - 哪个状态可以通过哪个事件转换到哪个状态
   - 画出状态转换图

4. **设置初始状态**
   - 确定系统启动时的初始状态

### 最佳实践

✅ **推荐做法:**
- 从简单场景开始练习
- 画出状态转换图
- 保持状态的单一职责
- 合理处理错误情况
- 考虑并发安全性

❌ **常见误区:**
- 状态过多导致复杂性增加
- 忘记处理无效的状态转换
- 在转换动作中放入过多逻辑
- 不考虑边界情况

## 🚨 错误处理

状态机会在以下情况返回错误:

1. **无效转换**: 当前状态无法通过指定事件进行转换
2. **动作执行失败**: 转换动作函数返回错误

```go
err := fsm.Trigger(INVALID_EVENT, nil)
if err != nil {
    fmt.Printf("转换失败: %v\n", err)
}
```

## 🎯 进阶方向

掌握基础概念后，可以探索以下进阶主题:

- **层次状态机**: 嵌套状态，支持状态的子状态
- **并行状态机**: 多个状态机协作
- **事件队列**: 异步事件处理
- **状态持久化**: 状态的保存和恢复
- **可视化**: 状态转换图的自动生成
- **测试**: 状态机的单元测试策略

## 📚 学习资源

- [有限状态机 - 维基百科](https://zh.wikipedia.org/wiki/有限状态机)
- [状态模式 - 设计模式](https://refactoringguru.cn/design-patterns/state)
- [Go语言官方文档](https://golang.org/doc/)

## 🤝 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 📄 许可证

本项目采用MIT许可证。

---

**开始你的状态机学习之旅吧！** 🚀