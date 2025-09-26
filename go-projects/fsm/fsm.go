package fsm

import (
	"fmt"
	"sync"
)

// 简化版本的有限状态机 - 专注于核心概念

// State 表示状态机中的一个状态
type State string

// Event 表示触发状态转换的事件
type Event string

// Action 表示状态转换时执行的动作（可选）
type Action func(from, to State, data interface{}) error

// Transition 表示状态转换规则
type Transition struct {
	From   State
	Event  Event
	To     State
	Action Action // 可选的转换动作
}

// FSM 有限状态机
type FSM struct {
	current     State                          // 当前状态
	transitions map[State]map[Event]Transition // 状态转换表
	mutex       sync.RWMutex                   // 并发安全锁
}

// NewFSM 创建新的状态机
func NewFSM(initial State) *FSM {
	return &FSM{
		current:     initial,
		transitions: make(map[State]map[Event]Transition),
	}
}

// AddTransition 添加状态转换规则
func (f *FSM) AddTransition(from State, event Event, to State, action Action) {
	f.mutex.Lock()
	defer f.mutex.Unlock()

	if f.transitions[from] == nil {
		f.transitions[from] = make(map[Event]Transition)
	}

	f.transitions[from][event] = Transition{
		From:   from,
		Event:  event,
		To:     to,
		Action: action,
	}
}

// Trigger 触发事件，尝试进行状态转换
func (f *FSM) Trigger(event Event, data interface{}) error {
	f.mutex.Lock()
	defer f.mutex.Unlock()

	// 查找当前状态下对应事件的转换规则
	if transitions, exists := f.transitions[f.current]; exists {
		if transition, exists := transitions[event]; exists {
			// 执行转换动作（如果有的话）
			if transition.Action != nil {
				if err := transition.Action(f.current, transition.To, data); err != nil {
					return fmt.Errorf("转换动作执行失败: %w", err)
				} else {
					fmt.Println("转换动作执行成功")
				}
			}

			// 更新当前状态
			f.current = transition.To
			return nil
		}
	}

	return fmt.Errorf("无效的状态转换: 从状态 %s 无法通过事件 %s 进行转换", f.current, event)
}

// Current 获取当前状态
func (f *FSM) Current() State {
	f.mutex.RLock()
	defer f.mutex.RUnlock()
	return f.current
}

// GetAvailableEvents 获取当前状态下可用的事件列表
func (f *FSM) GetAvailableEvents() []Event {
	f.mutex.RLock()
	defer f.mutex.RUnlock()

	var events []Event
	if transitions, exists := f.transitions[f.current]; exists {
		for event := range transitions {
			events = append(events, event)
		}
	}
	return events
}

// Reset 重置状态机到指定状态
func (f *FSM) Reset(state State) {
	f.mutex.Lock()
	defer f.mutex.Unlock()
	f.current = state
}

// String 返回状态机的字符串表示
func (f *FSM) String() string {
	f.mutex.RLock()
	defer f.mutex.RUnlock()
	return fmt.Sprintf("FSM{当前状态: %s}", f.current)
}
