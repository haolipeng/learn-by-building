#ifndef STATE_MACHINE_H //编译宏,防止重复包含
#define STATE_MACHINE_H

#include <stdio.h>

struct event {
    //事件类型
    int type;

    // 事件数据
    void* data;
};

struct state;

//状态转换的结构体
struct transition{
    //触发状态转换的事件
    int eventType;

    //待切换的下一个状态
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
    struct state* curState; //当前状态
    struct state* prevState;//之前状态
};

enum stateM_handleEventRetVals
{
   stateM_errArg = -2,
   stateM_errorStateReached, // 到达错误状态
   stateM_stateChanged, // 状态已改变
   stateM_stateLoopSelf, // 状态自循环
   stateM_noStateChange, // 无状态变化
   stateM_finalStateReached, // 到达最终状态
};

//初始化函数
void FSM_init(struct StateMachine* fsm, struct state* initState);

//状态机转换：处理事件
int FSM_handleEvent(struct StateMachine* fsm, struct event* event);

#endif