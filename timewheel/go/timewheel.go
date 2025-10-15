package timewheel

import (
	"container/list"
	"errors"
	"fmt"
	"sync"
	"time"
)

type Job func(key string)

type Task struct {
	key       string        //唯一的key
	job       Job           //任务执行函数
	executeAt time.Duration //执行时间
	slot      uint32        //槽位置
	circle    uint32        //圈数
}

type TimeWheel struct {
	interval     time.Duration //时间间隔
	ticker       *time.Ticker  //定时器
	currentSlot  uint32        //当前槽位置
	slotNum      uint32        //槽数量
	slots        []*list.List  //每个槽中存储任务的列表
	addTaskCh    chan *Task    //添加任务通道
	removeTaskCh chan *Task    //删除任务通道
	taskRecords  sync.Map      //任务记录表
	isRun        bool          //标记是否运行
}

// 外部接口
func NewTimeWheel(interval time.Duration, slotNum uint32) (*TimeWheel, error) {
	tw := &TimeWheel{
		interval:    interval,
		currentSlot: 0,
		slotNum:     slotNum,
		slots:       make([]*list.List, slotNum),
		addTaskCh:   make(chan *Task),
		taskRecords: sync.Map{},
		isRun:       false,
	}
	return tw, nil
}

// 外部接口
func (tw *TimeWheel) Start() error {
	//判断时间轮是否已经运行
	if tw.isRun {
		return errors.New("timewheel is already running")
	}
	//初始化槽列表
	for i := uint32(0); i < tw.slotNum; i++ {
		tw.slots[i] = list.New()
	}
	//初始化定时器
	tw.ticker = time.NewTicker(tw.interval)

	// 时间轮的主流程，处理各种业务逻辑
	go tw.run()

	//设置时间轮运行标志
	tw.isRun = true
	return nil
}

func (tw *TimeWheel) AddTask(key string, job Job, executeAt time.Duration) error {
	if key == "" {
		return errors.New("key is empty")
	}

	//判断下task是否已存在，存在则返回
	_, ok := tw.taskRecords.Load(key)
	if ok {
		return errors.New("key of job already exists")
	}

	task := &Task{
		key:       key,
		job:       job,
		executeAt: executeAt,
	}

	tw.addTaskCh <- task
	return nil
}

func (tw *TimeWheel) RemoveTask(key string) error {
	task, ok := tw.taskRecords.Load(key)
	if !ok {
		return errors.New("key of job not exists")
	}

	tw.removeTaskCh <- task.(*Task)
	return nil
}

// ///////////////////////////////内部方法/////////////////////////////////////
// 内部方法run
func (tw *TimeWheel) run() {
	for {
		select {
		case <-tw.ticker.C:
			tw.executeTask()
		case task := <-tw.addTaskCh:
			tw.addTask(task)
		case task := <-tw.removeTaskCh:
			tw.removeTask(task)
		}
	}
}

func (tw *TimeWheel) removeTask(task *Task) error {
	return nil
}

// 内部方法 addTask
func (tw *TimeWheel) addTask(task *Task) error {
	//slot, circle := tw.calSlotAndCircle(task.executeAt)
	//TODO:为了测试，这里设置slot为5，circle为0
	task.slot = 5
	task.circle = 0

	//将任务添加到指定槽位的链表中
	tw.slots[task.slot].PushBack(task)

	tw.taskRecords.Store(task.key, task)
	return nil
}

// 内部方法 执行任务
func (tw *TimeWheel) executeTask() error {
	//打印时间
	fmt.Println("executeTask", time.Now().Format("2006-01-02 15:04:05"))

	//获取当前槽
	taskList := tw.slots[tw.currentSlot]

	//遍历
	if taskList != nil {
		for ele := taskList.Front(); ele != nil; {
			next := ele.Next() //先保存下一个节点，避免删除后无法访问
			taskEle, _ := ele.Value.(*Task)
			go taskEle.job(taskEle.key) //异步执行任务

			//删除过期的元素
			taskList.Remove(ele)

			//从记录表中删除
			tw.taskRecords.Delete(taskEle.key)

			ele = next //使用保存的下一个节点
		}
	}

	//取模，移动到下一个槽
	tw.currentSlot = (tw.currentSlot + 1) % tw.slotNum
	return nil
}
