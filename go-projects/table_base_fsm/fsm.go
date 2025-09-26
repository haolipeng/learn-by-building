package table_base_fsm

import (
	"fmt"
	"sync"
)

type State int

type Event int

type Action func(from, to State, data interface{}) error

type Transition struct {
	From        State
	Event       Event
	To          State //待切换的下一个状态
	ActionIndex int   // 动作索引，去actionTable中取
}

// TableBasedFSM 基于状态转换表的有限状态机
type TableBasedFSM struct {
	// 状态转换表: [当前状态][事件] = Transition变量
	// 这是一个预填充的二维数组，所有状态转换在初始化时就确后
	transitionTable [][]Transition

	// 动作表: [动作索引] = 具体动作函数
	actionTable []Action

	// 当前状态
	currentState State

	// 并发安全
	mutex sync.RWMutex
}

// 常量定义：无效状态/事件/动作
const (
	INVALID_STATE  = -1
	INVALID_EVENT  = -1
	INVALID_ACTION = -1
)

// NewTableBasedFSMWithTable 使用预定义的二维数组创建状态机
// transitionTable: 状态转换表
// actionTable: 动作表
// initialState: 状态机初始状态
func NewTableBasedFSMWithTable(transitionTable [][]Transition, actionTable []Action, initialState State) *TableBasedFSM {
	// 参数验证
	if len(transitionTable) == 0 {
		panic("状态转换表不能为空")
	}
	if len(transitionTable[0]) == 0 {
		panic("状态转换表的事件维度不能为空")
	}
	if int(initialState) < 0 || int(initialState) >= len(transitionTable) {
		panic(fmt.Sprintf("初始状态 %d 超出范围 [0, %d)", initialState, len(transitionTable)))
	}

	return &TableBasedFSM{
		transitionTable: transitionTable,
		actionTable:     actionTable,
		currentState:    initialState,
	}
}

// Trigger 触发事件
func (fsm *TableBasedFSM) Trigger(event Event, data interface{}) error {
	fsm.mutex.Lock()
	defer fsm.mutex.Unlock()

	// 参数验证
	if event < 0 || int(event) >= len(fsm.transitionTable[0]) {
		return fmt.Errorf("无效的事件: %d (有效范围: 0-%d)", event, len(fsm.transitionTable[0])-1)
	}

	// 查找状态转换表
	transition := fsm.transitionTable[fsm.currentState][event]

	// 检查是否为有效转换（更清晰的判断逻辑）
	if !fsm.isValidTransition(transition) {
		return fmt.Errorf("无效的状态转换: 状态 %d 无法处理事件 %d", fsm.currentState, event)
	}

	// 执行动作
	if transition.ActionIndex != INVALID_ACTION {
		if transition.ActionIndex < 0 || transition.ActionIndex >= len(fsm.actionTable) {
			return fmt.Errorf("动作索引 %d 超出范围 [0, %d)", transition.ActionIndex, len(fsm.actionTable))
		}
		if err := fsm.actionTable[transition.ActionIndex](transition.From, transition.To, data); err != nil {
			return fmt.Errorf("动作执行失败: %w", err)
		}
	}

	// 更新状态
	fsm.currentState = transition.To
	return nil
}

// isValidTransition 检查转换是否有效
func (fsm *TableBasedFSM) isValidTransition(transition Transition) bool {
	// 检查转换是否为空（所有字段都为0值）
	return !(transition.From == 0 && transition.Event == 0 && transition.To == 0 && transition.ActionIndex == 0)
}

// CurrentState 获取当前状态
func (fsm *TableBasedFSM) CurrentState() State {
	fsm.mutex.RLock()
	defer fsm.mutex.RUnlock()
	return fsm.currentState
}

// GetAvailableEvents 获取当前状态下可用的事件
func (fsm *TableBasedFSM) GetAvailableEvents() []Event {
	fsm.mutex.RLock()
	defer fsm.mutex.RUnlock()

	var events []Event
	for event := 0; event < len(fsm.transitionTable[0]); event++ {
		transition := fsm.transitionTable[fsm.currentState][event]
		if fsm.isValidTransition(transition) {
			events = append(events, Event(event))
		}
	}
	return events
}

// Reset 重置到指定状态
func (fsm *TableBasedFSM) Reset(state State) {
	fsm.mutex.Lock()
	defer fsm.mutex.Unlock()
	fsm.currentState = state
}

// String 返回状态机的字符串表示
func (fsm *TableBasedFSM) String() string {
	fsm.mutex.RLock()
	defer fsm.mutex.RUnlock()
	return fmt.Sprintf("TableBasedFSM{当前状态: %d, 可用事件: %v}", fsm.currentState, fsm.GetAvailableEvents())
}
