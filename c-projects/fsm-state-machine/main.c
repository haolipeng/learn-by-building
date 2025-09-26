#include "state_machine.h"


// 售货机状态枚举
typedef enum {
    VENDING_IDLE = 0,          // 空闲等待
    VENDING_ITEM_SELECTED,     // 已选商品
    VENDING_COIN_INSERTED,     // 已投币
    VENDING_DISPENSING,        // 出货中
    VENDING_STATE_MAX
} vending_state_t;

// 售货机事件枚举
typedef enum {
    VENDING_SELECT_ITEM = 0,   // 选择商品
    VENDING_INSERT_COIN,       // 投币
    VENDING_DELIVER,           // 出货
    VENDING_RESET,             // 重置
    VENDING_EVENT_MAX
} vending_event_t;

// 售货机状态定义
static struct state idleState, itemSelectedState, coinInsertedState, dispensingState;

// 动作函数声明
static void selectItemAction(struct event* event);
static void insertCoinAction(struct event* event);
static void deliverItemAction(struct event* event);
static void resetAction(struct event* event);

// 空闲等待状态
static struct state idleState = {
    .name = "空闲等待",
    .entryState = NULL,
    .transitions = (struct transition[]){
       { VENDING_SELECT_ITEM, &itemSelectedState, selectItemAction },
    },
    .numTransitions = 1,
};

// 已选商品状态
static struct state itemSelectedState = {
    .name = "已选商品",
    .entryState = NULL,
    .transitions = (struct transition[]){
       { VENDING_INSERT_COIN, &coinInsertedState, insertCoinAction },
    },
    .numTransitions = 1,
};

// 已投币状态
static struct state coinInsertedState = {
    .name = "已投币",
    .entryState = NULL,
    .transitions = (struct transition[]){
       { VENDING_DELIVER, &dispensingState, deliverItemAction },
    },
    .numTransitions = 1,
};

// 出货中状态
static struct state dispensingState = {
    .name = "出货中",
    .entryState = NULL,
    .transitions = (struct transition[]){
       { VENDING_RESET, &idleState, resetAction },
    },
    .numTransitions = 1,
};

// 动作函数实现
static void selectItemAction(struct event* event) {
    printf("🛒 商品已选择，请投币\n");
}

static void insertCoinAction(struct event* event) {
    if (event && event->data) {
        double coinAmount = *(double*)event->data;
        printf("💰 投币 %.1f 元\n", coinAmount);
    } else {
        printf("💰 投币成功\n");
    }
}

static void deliverItemAction(struct event* event) {
    printf("📦 正在出货，请稍候...\n");
    printf("✅ 商品已出货，交易完成！\n");
}

static void resetAction(struct event* event) {
    printf("🔄 售货机重置，准备下次交易\n");
}

int main(int argc, char *argv[]) {
    printf("=== 自动售货机状态机演示 ===\n");
    printf("操作说明:\n");
    printf("  1 - 选择商品\n");
    printf("  2 - 投币\n");
    printf("  3 - 出货\n");
    printf("  4 - 重置\n");
    printf("  q - 退出\n");
    printf("==============================\n");
    
    //初始化状态机
    struct StateMachine m;
    FSM_init( &m, &idleState);
    
    printf("初始状态: %s\n", m.curState->name);

    char input[100];
    while (1) {
        printf("\n当前状态: %s\n", m.curState->name);
        printf("请选择操作 (1-4, q): ");
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // EOF
        }
        
        char choice = input[0];
        int result = stateM_noStateChange;
        
        switch(choice) {
            case '1': {
                // 选择商品
                struct event event = { VENDING_SELECT_ITEM, NULL };
                result = FSM_handleEvent(&m, &event);
                break;
            }
            case '2': {
                // 投币
                double coinAmount = 1.0; // 默认投币金额
                struct event event = { VENDING_INSERT_COIN, &coinAmount };
                result = FSM_handleEvent(&m, &event);
                break;
            }
            case '3': {
                // 出货
                struct event event = { VENDING_DELIVER, NULL };
                result = FSM_handleEvent(&m, &event);
                break;
            }
            case '4': {
                // 重置
                struct event event = { VENDING_RESET, NULL };
                result = FSM_handleEvent(&m, &event);
                break;
            }
            case 'q':
                printf("👋 再见！\n");
                return 0;
            default:
                printf("❌ 无效选择，请输入 1-4 或 q\n");
                continue;
        }
    }

    return 0;
}