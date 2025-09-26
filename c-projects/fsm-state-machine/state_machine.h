#ifndef STATE_MACHINE_H //编译宏
#define STATE_MACHINE_H

#include <stdio.h>

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

struct event {
    //事件类型
    vending_event_t type;
    // 事件数据（用于投币金额等）
    void* data;
};

struct state;

//状态转换的结构体
struct transition{
    //触发状态转换的事件
    vending_event_t eventType;

    //下一个状态
    struct state* nextState;
    
    // 动作函数指针（可选）
    void (*action)(struct event* event);
};

//状态
struct state {
    const char* name; // 状态名称，用于调试和显示
    struct state* entryState;
    
    struct transition* transitions; //状态转换数组
    int numTransitions;//状态转换数组大小
};

struct StateMachine {
    struct state* curState;
    struct state* prevState;
};

enum stateM_handleEventRetVals
{
   stateM_errArg = -2,
   stateM_errorStateReached,
   stateM_stateChanged,
   stateM_stateLoopSelf,
   stateM_noStateChange,
   stateM_finalStateReached,
};

//初始化函数
void FSM_init(struct StateMachine* fsm, struct state* initState);

//状态机转换：处理事件
int FSM_handleEvent(struct StateMachine* fsm, struct event* event);

#endif