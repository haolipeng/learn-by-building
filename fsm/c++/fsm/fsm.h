#ifndef FSM_H
#define FSM_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

// 前向声明
class State;
class Event;
class Transition;
class StateMachine;

// 事件基类
class Event {
public:
    Event(int type, void* data = NULL) : type_(type), data_(data) {}
    virtual ~Event() {}
    
    int getType() const { return type_; }
    void* getData() const { return data_; }
    
private:
    int type_;
    void* data_;    //这个data数据是干啥的？
};

// 状态基类
class State {
public:
    State(const std::string& name) : name_(name) {}
    virtual ~State() {}
    
    const std::string& getName() const { return name_; }
    
    // 添加转换
    void addTransition(int eventType, State* nextState, void (*action)(const Event&) = NULL);
    
    // 获取转换
    Transition* getTransition(int eventType);
    
    // 获取所有可能的转换
    std::map<int, Transition*>& getTransitions() { return transitions_; }
    
private:
    std::string name_;
    std::map<int, Transition*> transitions_;
};

// 转换类
class Transition {
public:
    Transition(int eventType, State* nextState, void (*action)(const Event&) = NULL)
        : eventType_(eventType), nextState_(nextState), action_(action) {}
    
    int getEventType() const { return eventType_; }
    State* getNextState() const { return nextState_; }
    void (*getAction() const)(const Event&) { return action_; }
    
private:
    int eventType_;
    State* nextState_;
    void (*action_)(const Event&);
};

// 状态机类
class StateMachine {
public:
    StateMachine(State* initialState);
    ~StateMachine();
    
    // 处理事件
    int handleEvent(const Event& event);
    
    // 获取当前状态
    State* getCurrentState() const { return currentState_; }
    
    // 获取前一个状态
    State* getPreviousState() const { return previousState_; }
    
    // 重置状态机
    void reset(State* state);
    
    // 检查是否可以处理事件
    bool canHandleEvent(int eventType) const;
    
    // 获取当前状态下可用的事件
    std::vector<int> getAvailableEvents() const;
    
    // 状态机状态
    enum StateMachineResult {
        STATE_ERROR_ARG = -2,
        STATE_ERROR_REACHED,
        STATE_CHANGED,
        STATE_LOOP_SELF,
        STATE_NO_CHANGE,
        STATE_FINAL_REACHED
    };
    
private:
    void goToErrorState();
    
    State* currentState_;
    State* previousState_;
};

#endif // FSM_H