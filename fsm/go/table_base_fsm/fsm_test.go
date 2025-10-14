package table_base_fsm

import (
	"fmt"
	"strings"
	"testing"
)

// 售货机状态枚举
const (
	VENDING_IDLE          State = 0 // 空闲等待
	VENDING_ITEM_SELECTED State = 1 // 已选商品
	VENDING_COIN_INSERTED State = 2 // 已投币
	VENDING_DISPENSING    State = 3 // 出货中
	VENDING_STATE_MAX     State = 4 // 错误
)

// 售货机事件枚举
const (
	VENDING_SELECT_ITEM Event = 0 // 选择商品
	VENDING_INSERT_COIN Event = 1 // 投币
	VENDING_DELIVER     Event = 2 // 出货
	VENDING_RESET       Event = 3 // 重置
	VENDING_EVENT_MAX   Event = 4 // 错误
)

// VendingMachineTransitionTable 售货机状态转换表
var VendingMachineTransitionTable = [][]Transition{
	// 空闲等待状态 -> [选择商品, 投币, 出货, 重置]
	{
		{From: VENDING_IDLE, Event: VENDING_SELECT_ITEM, To: VENDING_ITEM_SELECTED, ActionIndex: 0},
		{},
		{},
		{},
	},
	// 已选商品状态 -> [选择商品, 投币, 出货, 重置]
	{
		{},
		{From: VENDING_ITEM_SELECTED, Event: VENDING_INSERT_COIN, To: VENDING_COIN_INSERTED, ActionIndex: 1},
		{},
		{},
	},
	// 已投币状态 -> [选择商品, 投币, 出货, 重置]
	{
		{},
		{},
		{From: VENDING_COIN_INSERTED, Event: VENDING_DELIVER, To: VENDING_DISPENSING, ActionIndex: 0},
		{},
	},
	// 出货中状态 -> [选择商品, 投币, 出货, 重置]
	{
		{},
		{},
		{},
		{From: VENDING_DISPENSING, Event: VENDING_RESET, To: VENDING_IDLE, ActionIndex: 0},
	},
}

// VendingMachineActionTable 售货机动作表
var VendingMachineActionTable = []Action{
	// 索引0: 日志动作
	func(from, to State, data interface{}) error {
		fmt.Printf("🔄 售货机: %d -> %d\n", from, to)
		return nil
	},
	// 索引1: 投币验证动作
	func(from, to State, data interface{}) error {
		// 检查投币金额
		if data == nil {
			return fmt.Errorf("投币金额不能为空")
		}

		coinAmount, ok := data.(float64)
		if !ok {
			return fmt.Errorf("投币金额格式错误")
		}

		itemPrice := 3.0 // 商品价格3元
		if coinAmount < itemPrice {
			return fmt.Errorf("投币金额不足，需要 %.1f 元，当前投币 %.1f 元", itemPrice, coinAmount)
		}

		// 计算找零
		change := coinAmount - itemPrice
		if change > 0 {
			fmt.Printf("  💰 找零: %.1f 元\n", change)
		}

		return nil
	},
}

// TestTableBasedFSM 测试基于表的售货机状态机
func TestTableBasedFSM(t *testing.T) {
	// 定义售货机状态常量
	const (
		IDLE          = 0 // 空闲等待
		ITEM_SELECTED = 1 // 已选商品
		COIN_INSERTED = 2 // 已投币
		DISPENSING    = 3 // 出货中
	)

	// 定义售货机事件常量
	const (
		SELECT_ITEM = 0 // 选择商品
		INSERT_COIN = 1 // 投币
		DELIVER     = 2 // 出货
		RESET       = 3 // 重置
	)

	// 使用预定义的售货机状态机
	vendingFSM := NewTableBasedFSMWithTable(VendingMachineTransitionTable, VendingMachineActionTable, VENDING_IDLE)

	// 测试正常购买流程
	t.Run("正常购买流程", func(t *testing.T) {
		// 重置到初始状态
		vendingFSM.Reset(IDLE)

		// 测试初始状态
		if vendingFSM.CurrentState() != IDLE {
			t.Errorf("期望初始状态为%d，但得到 %d", IDLE, vendingFSM.CurrentState())
		}

		// 1. 选择商品
		err := vendingFSM.Trigger(SELECT_ITEM, nil) // 选择商品
		if err != nil {
			t.Errorf("选择商品失败: %v", err)
		}
		if vendingFSM.CurrentState() != ITEM_SELECTED {
			t.Errorf("期望状态为%d，但得到 %d", ITEM_SELECTED, vendingFSM.CurrentState())
		}

		// 2. 投币（验证金额）
		err = vendingFSM.Trigger(INSERT_COIN, 5.0) // 投币5元，商品3元
		if err != nil {
			t.Errorf("投币失败: %v", err)
		}
		if vendingFSM.CurrentState() != COIN_INSERTED {
			t.Errorf("期望状态为%d，但得到 %d", COIN_INSERTED, vendingFSM.CurrentState())
		}

		// 3. 出货
		err = vendingFSM.Trigger(DELIVER, nil) // 出货
		if err != nil {
			t.Errorf("出货失败: %v", err)
		}
		if vendingFSM.CurrentState() != DISPENSING {
			t.Errorf("期望状态为%d，但得到 %d", DISPENSING, vendingFSM.CurrentState())
		}

		// 4. 完成交易
		err = vendingFSM.Trigger(RESET, nil) // 重置
		if err != nil {
			t.Errorf("重置失败: %v", err)
		}
		if vendingFSM.CurrentState() != IDLE {
			t.Errorf("期望最终状态为%d，但得到 %d", IDLE, vendingFSM.CurrentState())
		}
	})

	// 测试错误流程 - 在错误状态下尝试操作
	t.Run("错误流程测试", func(t *testing.T) {
		vendingFSM.Reset(IDLE)

		// 在空闲状态下直接投币应该失败
		err := vendingFSM.Trigger(INSERT_COIN, 5.0) // 投币
		if err == nil {
			t.Error("期望在空闲状态下投币应该失败")
		}

		// 在空闲状态下直接出货应该失败
		err = vendingFSM.Trigger(DELIVER, nil) // 出货
		if err == nil {
			t.Error("期望在空闲状态下出货应该失败")
		}
	})

	// 测试金额不足的情况
	t.Run("金额不足测试", func(t *testing.T) {
		vendingFSM.Reset(IDLE)

		// 选择商品
		err := vendingFSM.Trigger(SELECT_ITEM, nil) // 选择商品
		if err != nil {
			t.Errorf("选择商品失败: %v", err)
		}

		// 投币金额不足
		err = vendingFSM.Trigger(INSERT_COIN, 1.0) // 只投1元，商品需要3元
		if err == nil {
			t.Error("期望投币金额不足时应该失败")
		}
		if err != nil && !strings.Contains(err.Error(), "投币金额不足") {
			t.Errorf("期望错误信息包含'投币金额不足'，但得到: %v", err)
		}

		// 状态应该仍然是已选商品
		if vendingFSM.CurrentState() != ITEM_SELECTED {
			t.Errorf("期望状态仍为%d，但得到 %d", ITEM_SELECTED, vendingFSM.CurrentState())
		}
	})

	// 测试投币金额格式错误
	t.Run("投币格式错误测试", func(t *testing.T) {
		vendingFSM.Reset(IDLE)

		// 选择商品
		err := vendingFSM.Trigger(SELECT_ITEM, nil) // 选择商品
		if err != nil {
			t.Errorf("选择商品失败: %v", err)
		}

		// 投币数据为空
		err = vendingFSM.Trigger(INSERT_COIN, nil) // 投币
		if err == nil {
			t.Error("期望投币数据为空时应该失败")
		}

		// 投币数据格式错误
		err = vendingFSM.Trigger(INSERT_COIN, "invalid") // 投币
		if err == nil {
			t.Error("期望投币数据格式错误时应该失败")
		}
	})
}
