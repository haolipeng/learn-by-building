#include "fsm.h"
#include <algorithm>

// State类实现
void State::addTransition(int eventType, State* nextState, void (*action)(const Event&)) {
    // 如果已存在该事件类型的转换，先删除旧的
    std::map<int, Transition*>::iterator it = transitions_.find(eventType);
    if (it != transitions_.end()) {
        delete it->second;
    }
    // 添加新的转换
    transitions_[eventType] = new Transition(eventType, nextState, action);
}

Transition* State::getTransition(int eventType) {
    std::map<int, Transition*>::iterator it = transitions_.find(eventType);
    if (it != transitions_.end()) {
        return it->second;
    }
    return NULL;
}

// StateMachine类实现
StateMachine::StateMachine(State* initialState) 
    : currentState_(initialState), previousState_(NULL) {
    if (!currentState_) {
        goToErrorState();
    }
}

StateMachine::~StateMachine() {
    // 注意：这里不删除状态，因为状态通常由外部管理
}

void StateMachine::goToErrorState() {
    previousState_ = currentState_;
    currentState_ = NULL;
}

int StateMachine::handleEvent(const Event& event) {
    // 参数检查
    if (!currentState_) {
        goToErrorState();
        return STATE_ERROR_REACHED;
    }
    
    // 获取转换
    Transition* transition = currentState_->getTransition(event.getType());
    if (!transition) {
        return STATE_NO_CHANGE;
    }
    
    // 检查下一个状态
    if (!transition->getNextState()) {
        goToErrorState();
        return STATE_ERROR_REACHED;
    }
    
    // 执行动作
    if (transition->getAction()) {
        transition->getAction()(event);
    }
    
    // 更新状态
    previousState_ = currentState_;
    currentState_ = transition->getNextState();
    
    // 检查自循环
    if (currentState_ == previousState_) {
        return STATE_LOOP_SELF;
    }
    
    // 检查是否为最终状态（没有转换的状态）
    if (currentState_->getTransitions().empty()) {
        return STATE_FINAL_REACHED;
    }
    
    return STATE_CHANGED;
}

void StateMachine::reset(State* state) {
    currentState_ = state;
    previousState_ = NULL;
}

bool StateMachine::canHandleEvent(int eventType) const {
    if (!currentState_) {
        return false;
    }
    return currentState_->getTransition(eventType) != NULL;
}

std::vector<int> StateMachine::getAvailableEvents() const {
    std::vector<int> events;
    if (!currentState_) {
        return events;
    }
    
    const std::map<int, Transition*>& transitions = currentState_->getTransitions();
    for (std::map<int, Transition*>::const_iterator it = transitions.begin();
         it != transitions.end(); ++it) {
        events.push_back(it->first);
    }
    
    return events;
}