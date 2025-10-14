#include "fsm.h"
#include <iostream>
#include <string>

// 售货机事件类型
enum VendingEventType {
    VENDING_SELECT_ITEM = 0,
    VENDING_INSERT_COIN,
    VENDING_DELIVER,
    VENDING_RESET
};

// 售货机状态类
class VendingState : public State {
public:
    VendingState(const std::string& name) : State(name) {}
};

// 使用简单的Event基类

// 动作函数
void selectItemAction(const Event& event) {
    std::cout << "🛒 商品已选择，请投币" << std::endl;
}

void insertCoinAction(const Event& event) {
    if (event.getData()) {
        double coinAmount = *(static_cast<double*>(event.getData()));
        std::cout << "💰 投币 " << coinAmount << " 元" << std::endl;
    } else {
        std::cout << "💰 投币成功" << std::endl;
    }
}

void deliverItemAction(const Event& event) {
    std::cout << "📦 正在出货，请稍候..." << std::endl;
    std::cout << "✅ 商品已出货，交易完成！" << std::endl;
}

void resetAction(const Event& event) {
    std::cout << "🔄 售货机重置，准备下次交易" << std::endl;
}

int main() {
    std::cout << "=== C++自动售货机状态机演示 ===" << std::endl;
    std::cout << "操作说明:" << std::endl;
    std::cout << "  1 - 选择商品" << std::endl;
    std::cout << "  2 - 投币" << std::endl;
    std::cout << "  3 - 出货" << std::endl;
    std::cout << "  4 - 重置" << std::endl;
    std::cout << "  q - 退出" << std::endl;
    std::cout << "================================" << std::endl;
    
    // 创建状态
    VendingState idleState("空闲等待");
    VendingState itemSelectedState("已选商品");
    VendingState coinInsertedState("已投币");
    VendingState dispensingState("出货中");
    
    // 配置状态转换
    idleState.addTransition(VENDING_SELECT_ITEM, &itemSelectedState, selectItemAction);
    itemSelectedState.addTransition(VENDING_INSERT_COIN, &coinInsertedState, insertCoinAction);
    coinInsertedState.addTransition(VENDING_DELIVER, &dispensingState, deliverItemAction);
    dispensingState.addTransition(VENDING_RESET, &idleState, resetAction);
    
    // 创建状态机
    StateMachine vendingMachine(&idleState);
    
    std::cout << "初始状态: " << vendingMachine.getCurrentState()->getName() << std::endl;
    
    std::string input;
    while (true) {
        std::cout << std::endl << "当前状态: " << vendingMachine.getCurrentState()->getName() << std::endl;
        std::cout << "请选择操作 (1-4, q): ";
        
        if (!std::getline(std::cin, input)) {
            break; // EOF
        }
        
        if (input.empty()) {
            continue;
        }
        
        char choice = input[0];
        int result = StateMachine::STATE_NO_CHANGE;
        
        switch (choice) {
            case '1': {
                // 选择商品
                Event event(VENDING_SELECT_ITEM);
                result = vendingMachine.handleEvent(event);
                break;
            }
            case '2': {
                // 投币
                double coinAmount = 1.0; // 默认投币金额
                Event event(VENDING_INSERT_COIN, &coinAmount);
                result = vendingMachine.handleEvent(event);
                break;
            }
            case '3': {
                // 出货
                Event event(VENDING_DELIVER);
                result = vendingMachine.handleEvent(event);
                break;
            }
            case '4': {
                // 重置
                Event event(VENDING_RESET);
                result = vendingMachine.handleEvent(event);
                break;
            }
            case 'q':
            case 'Q':
                std::cout << "👋 再见！" << std::endl;
                return 0;
            default:
                std::cout << "❌ 无效选择，请输入 1-4 或 q" << std::endl;
                continue;
        }
        
        // 打印状态转换结果
        switch (result) {
            case StateMachine::STATE_CHANGED:
                std::cout << "✅ 状态已改变" << std::endl;
                break;
            case StateMachine::STATE_LOOP_SELF:
                std::cout << "🔄 状态自循环" << std::endl;
                break;
            case StateMachine::STATE_NO_CHANGE:
                std::cout << "⚠️  无状态变化" << std::endl;
                break;
            case StateMachine::STATE_ERROR_REACHED:
                std::cout << "❌ 到达错误状态" << std::endl;
                break;
            case StateMachine::STATE_FINAL_REACHED:
                std::cout << "🏁 到达最终状态" << std::endl;
                break;
            default:
                std::cout << "❓ 未知结果: " << result << std::endl;
        }
    }
    
    return 0;
}
