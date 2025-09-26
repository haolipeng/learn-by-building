#include "state_machine.h"

//初始化函数
void FSM_init(struct StateMachine *fsm, struct state* initState)
{
    if ( !fsm )
      return;

   fsm->curState = initState;
   fsm->prevState = NULL;
}

static void goToErrorState( struct StateMachine *fsm,
    struct event *const event )
{
    fsm->prevState = fsm->curState;
    fsm->curState = NULL;
}

static struct transition *getTransition( struct StateMachine *fsm,
    struct state *state, struct event *const event )
{
    size_t i;

    //遍历state状态的转换数组transitions
    for ( i = 0; i < state->numTransitions; ++i )
    {
        struct transition *t = &state->transitions[ i ];
        //查找事件的type类型是否匹配，如果匹配则返回
        if ( t->eventType == event->type )
        {
            return t;
        }
    }

    /* No transitions found for given event for given state: */
    return NULL;
}

//状态机转换：处理事件
int FSM_handleEvent(struct StateMachine* fsm, struct event* event)
{
    //判断入参fsm和event是否为空，为空时直接返回stateM_errArg
   if ( !fsm || !event )
        return stateM_errArg;

    //判断当前状态是否为空，为空则直接返回stateM_errorStateReached
    if ( !fsm->curState )
    {
        goToErrorState( fsm, event );
        return stateM_errorStateReached;
    }

    //判断当前状态的转换数组是否为空，为空则直接返回stateM_noStateChange
    if ( !fsm->curState->numTransitions )
        return stateM_noStateChange;

    struct state *nextState = fsm->curState;
    do {
        //获取当前状态的transition
        struct transition *transition = getTransition( fsm, nextState, event );

        //如果transition为空，说明没有找到匹配的转换
        if ( !transition )
        {
            return stateM_noStateChange;
        }

        //如果transition的nextState为空，则直接返回stateM_errorStateReached
        if ( !transition->nextState )
        {
            goToErrorState( fsm, event );
            return stateM_errorStateReached;
        }

        //成功获取transition的下一个状态
        nextState = transition->nextState;

        printf("状态转换: %s -> %s\n", fsm->curState->name, nextState->name);

        // 执行动作（如果存在）
        if (transition->action) {
            transition->action(event);
        }

        //保存状态信息
        fsm->prevState = fsm->curState;
        fsm->curState = nextState; //更新当前状态为最新状态
        
        //这个判断是否自循环了，如果自循环了，则返回stateM_stateLoopSelf
        if ( fsm->curState == fsm->prevState )
            return stateM_stateLoopSelf;

        if ( !fsm->curState->numTransitions )
            return stateM_finalStateReached;

        return stateM_stateChanged;
    } while ( nextState );

    return stateM_noStateChange;
}