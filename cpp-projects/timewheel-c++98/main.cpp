#include <iostream>
#include <vector>
#include <list>
#include <pthread.h>
#include <unistd.h>   // usleep

// ----------------- 定时器对象 ------------------
typedef void (*TimerCallback)(void*);

struct Timer {
    int ticks;             // 剩余多少个 tick 到期
    TimerCallback cb;      // 回调函数指针
    void* arg;             // 回调参数
};

// ----------------- 单层时间轮 ------------------
class TimerWheel {
public:
    TimerWheel(int wheelSize, int tickMs)
        : wheelSize_(wheelSize),
          tickMs_(tickMs),
          currentSlot_(0),
          stop_(false)
    {
        slots_.resize(wheelSize_);
        pthread_mutex_init(&mtx_, NULL);
    }

    ~TimerWheel() {
        stop();
        pthread_mutex_destroy(&mtx_);
    }

    // 添加一个定时器: 延迟 delayMs 毫秒后执行
    void addTimer(int delayMs, TimerCallback cb, void* arg) {
        if (delayMs <= 0) delayMs = tickMs_; 

        int ticks = delayMs / tickMs_;
        int slot = (currentSlot_ + ticks) % wheelSize_;

        pthread_mutex_lock(&mtx_);
        Timer t;
        t.ticks = ticks;
        t.cb = cb;
        t.arg = arg;
        slots_[slot].push_back(t);
        pthread_mutex_unlock(&mtx_);
    }

    // 启动工作线程
    void start() {
        stop_ = false;
        pthread_create(&worker_, NULL, workerThread, this);
    }

    void stop() {
        stop_ = true;
        if (worker_) {
            pthread_join(worker_, NULL);
            worker_ = 0;
        }
    }

private:
    static void* workerThread(void* arg) {
        TimerWheel* tw = (TimerWheel*)arg;
        while (!tw->stop_) {
            usleep(tw->tickMs_ * 1000);  // 毫秒转微秒
            tw->tick();
        }
        return NULL;
    }

    void tick() {
        std::list<Timer> ready;

        pthread_mutex_lock(&mtx_);
        std::list<Timer>& slotList = slots_[currentSlot_];
        for (std::list<Timer>::iterator it = slotList.begin(); it != slotList.end(); ) {
            it->ticks -= 1;
            if (it->ticks <= 0) {
                ready.push_back(*it);
                it = slotList.erase(it);
            } else {
                ++it;
            }
        }
        currentSlot_ = (currentSlot_ + 1) % wheelSize_;
        pthread_mutex_unlock(&mtx_);

        // 锁外执行回调
        for (std::list<Timer>::iterator it = ready.begin(); it != ready.end(); ++it) {
            if (it->cb) {
                it->cb(it->arg);
            }
        }
    }

private:
    int wheelSize_;
    int tickMs_;
    int currentSlot_;

    std::vector<std::list<Timer> > slots_;
    volatile bool stop_;
    pthread_t worker_;
    pthread_mutex_t mtx_;
};

// ----------------- 示例回调 ------------------
void taskPrint(void* arg) {
    const char* s = (const char*)arg;
    std::cout << s << std::endl;
}

int main() {
    TimerWheel wheel(8, 1000); // 8个槽, 每个tick = 1秒
    wheel.start();

    wheel.addTimer(2000, taskPrint, (void*)"Task 1 after 2s");
    wheel.addTimer(5000, taskPrint, (void*)"Task 2 after 5s");
    wheel.addTimer(8000, taskPrint, (void*)"Task 3 after 8s");
    wheel.addTimer(9000, taskPrint, (void*)"Task 4 after 9s");

    sleep(12);

    wheel.stop();
    return 0;
}