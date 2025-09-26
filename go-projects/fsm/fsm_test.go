package fsm

import (
	"fmt"
	"testing"
)

// TestTrafficLight 测试红绿灯状态机
func TestTrafficLight(t *testing.T) {
	// 定义红绿灯状态
	const (
		RED    State = "红灯"
		YELLOW State = "黄灯"
		GREEN  State = "绿灯"
	)

	// 定义事件
	const (
		NEXT Event = "下一个"
	)

	// 创建红绿灯状态机
	trafficLight := NewFSM(RED)

	// 定义循环转换: 红灯 -> 绿灯 -> 黄灯 -> 红灯
	trafficLight.AddTransition(RED, NEXT, GREEN, nil)
	trafficLight.AddTransition(GREEN, NEXT, YELLOW, nil)
	trafficLight.AddTransition(YELLOW, NEXT, RED, nil)

	// 测试循环转换
	expectedStates := []State{RED, GREEN, YELLOW, RED, GREEN, YELLOW}

	for i, expectedState := range expectedStates {
		if trafficLight.Current() != expectedState {
			t.Errorf("步骤 %d: 期望状态为 %s，但得到 %s", i, expectedState, trafficLight.Current())
		}

		if i < len(expectedStates)-1 { // 不在最后一步触发
			err := trafficLight.Trigger(NEXT, nil)
			if err != nil {
				t.Errorf("步骤 %d: 状态转换失败: %v", i, err)
			}
		}
	}
}

// 定义商品价格
var itemPrice = 3.0 // 商品价格3元

func verifyCoinCount(from, to State, data interface{}) error {
	// 检查投币金额
	if data == nil {
		return fmt.Errorf("投币金额不能为空")
	}

	coinAmount, ok := data.(float64)
	if !ok {
		return fmt.Errorf("投币金额格式错误")
	}

	if coinAmount < itemPrice {
		return fmt.Errorf("投币金额不足，需要 %.1f 元，当前投币 %.1f 元", itemPrice, coinAmount)
	}

	return nil
}

// TestVendingMachine 测试自动售货机状态机
func TestVendingMachine(t *testing.T) {
	// 定义售货机状态
	const (
		IDLE          State = "空闲等待"
		ITEM_SELECTED State = "已选商品"
		COIN_INSERTED State = "已投币"
		DISPENSING    State = "出货中"
	)

	// 定义售货机事件
	const (
		SELECT_ITEM Event = "选择商品"
		INSERT_COIN Event = "投币"
		DELIVER     Event = "出货"
		RESET       Event = "重置"
	)

	// 创建售货机状态机
	vendingFSM := NewFSM(IDLE)

	// 定义状态转换规则 - 正确的售货机流程
	vendingFSM.AddTransition(IDLE, SELECT_ITEM, ITEM_SELECTED, nil) // 空闲状态 -> 已选商品状态

	// 投币时验证金额是否足够,verifyCoinCount 是硬币金额校验函数
	vendingFSM.AddTransition(ITEM_SELECTED, INSERT_COIN, COIN_INSERTED, verifyCoinCount)

	vendingFSM.AddTransition(COIN_INSERTED, DELIVER, DISPENSING, nil) // 已投币状态 -> 出货中状态
	vendingFSM.AddTransition(DISPENSING, RESET, IDLE, nil)            // 出货中状态 -> 空闲状态

	// 测试正常购买流程
	t.Run("正常购买流程", func(t *testing.T) {
		// 重置到初始状态
		vendingFSM.Reset(IDLE)

		// 测试初始状态
		if vendingFSM.Current() != IDLE {
			t.Errorf("期望初始状态为 %s，但得到 %s", IDLE, vendingFSM.Current())
		}

		// 1. 选择商品
		err := vendingFSM.Trigger(SELECT_ITEM, nil)
		if err != nil {
			t.Errorf("选择商品失败: %v", err)
		}
		if vendingFSM.Current() != ITEM_SELECTED {
			t.Errorf("期望状态为 %s，但得到 %s", ITEM_SELECTED, vendingFSM.Current())
		}

		// 2. 投币（验证金额）
		err = vendingFSM.Trigger(INSERT_COIN, 5.0) // 投币5元，商品3元
		if err != nil {
			t.Errorf("投币失败: %v", err)
		}
		if vendingFSM.Current() != COIN_INSERTED {
			t.Errorf("期望状态为 %s，但得到 %s", COIN_INSERTED, vendingFSM.Current())
		}

		// 3. 出货
		err = vendingFSM.Trigger(DELIVER, nil)
		if err != nil {
			t.Errorf("出货失败: %v", err)
		}
		if vendingFSM.Current() != DISPENSING {
			t.Errorf("期望状态为 %s，但得到 %s", DISPENSING, vendingFSM.Current())
		}

		// 4. 完成交易
		err = vendingFSM.Trigger(RESET, nil)
		if err != nil {
			t.Errorf("重置失败: %v", err)
		}
		if vendingFSM.Current() != IDLE {
			t.Errorf("期望最终状态为 %s，但得到 %s", IDLE, vendingFSM.Current())
		}
	})

	// 测试错误流程 - 在错误状态下尝试操作
	t.Run("错误流程测试", func(t *testing.T) {
		vendingFSM.Reset(IDLE)

		// 在空闲状态下直接投币应该失败
		err := vendingFSM.Trigger(INSERT_COIN, 5.0)
		if err == nil {
			t.Error("期望在空闲状态下投币应该失败")
		}

		// 在空闲状态下直接出货应该失败
		err = vendingFSM.Trigger(DELIVER, nil)
		if err == nil {
			t.Error("期望在空闲状态下出货应该失败")
		}
	})
}
