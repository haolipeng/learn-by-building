# 一、时间轮概述

## **原理：**

时间轮(Timing Wheel)是一种环形的数据结构，就像一个时钟可以分成很多格子（Slot) 底层采用数组实现，每个格子代表时间的间隔 (Interval)，存放的是具体定时任务列表（TaskList），TaskList是一个环形双向链表，链表中的每个元素都是定时任务 Task。



### 单级轮

![img](https://pic1.zhimg.com/v2-97e2d8a671f3c02c2fc3907bb98c3420_1440w.jpg)

上图中：是一个槽数为12、时间间隔interval等于1s的时间轮、当前currentTime = 3，slot = 3 的链表中存有4个任务项task



新增一个5s的任务后，会怎么样呢

如果当前的指针 currentTime 指向的是3，此时如果插入一个5s的任务进来，那么新来的任务会存放到时间格8中。

![img](https://pica.zhimg.com/v2-be8886803ef8c366e3d30036d8b17818_1440w.jpg)

### 层级轮

上面讲的都是简单单级时间轮，如果时间跨度查过了时间轮的刻度slot、比如添加一个15秒之后执行的任务，单个轮盘就无法满足我们的需求。我们就要考虑使用别的方案了！



在确定如何实现之前，先捋一捋多级轮中时间和轮次的关系:

在单次轮中我们假设有m个slot，形成一个数组(环形队列的底层用数组表示)，每个slot表示的时间间隔是 t，那么能个表示的时间范围就是 m * t，，获取slot上的tasklist，可以用slot[i]表示，i 是slot数组的下标。

![img](https://picx.zhimg.com/v2-07c3486b62e63146eb19137081327e1b_1440w.jpg)

如何实现层级轮呢？

**task关联circle**

每个任务添加一个circle (圈数) 的参数，未超过时间轮单轮刻度那么圈数circle = 0，超过的话就通过计算得到circle的值。

例如某个任务需要16s之后执行，那么该任务的circle = 1，计算得到的表盘位置slot = 4，也就是该任务需要在表盘走一圈以后，在位置4处执行。如果表盘指针刚好前进到该slot，该处的任务列表中的circle都减1，直到slot = 4的任务链表中的任务circle = 0，才执行该任务。

相比较而言个人更倾向于基于在任务上添加circle参数的方式，这种方式需要控制的是任务上的circle的值，本文也是基于这种方式去实现的。



## 实现

梳理下时间轮算法的流程：

- 建立环状数据结构（底层数组实现）
- 确定数据结构的刻度(slot)、每个刻度对应的时间 interval、单圈能表示的时间等于 slot * interval
- 创建任务，根据任务的延迟执行时间，确定刻度slot和圈数circle
- 在刻度上挂上要执行的定时任务链表
- 执行对应刻度(slot)链表上的任务



## 时间轮主要解决的问题

### 1. 大量定时任务的性能问题 ⏰
传统的定时器方案（如每个任务一个Timer）在面对海量定时任务时存在以下问题：
- 每个定时任务都要创建一个独立的定时器，内存开销大
- 大量定时器的管理和调度会消耗大量CPU资源（即使使用epoll + timerfd，cpu消耗也不小）
- 时间复杂度高（每次插入/删除任务可能需要O(log n)）

**时间轮的解决方案**：
- 只需要**一个定时器**（ticker）驱动整个时间轮
- 使用环形数组 + 链表的数据结构，插入删除任务时间复杂度为O(1)
- 通过槽位（slot）和圈数（circle）的设计，高效管理不同延迟时间的任务

### 2. 延迟任务的高效调度 📋
对于需要在未来某个时间点执行的任务，时间轮通过以下方式实现高效调度：

```
示例：时间间隔1秒，10个槽位
- 5秒后执行的任务 → slot=5, circle=0
- 15秒后执行的任务 → slot=5, circle=1
- 25秒后执行的任务 → slot=5, circle=2
```

### 3. 实际应用场景
时间轮广泛应用于：
- **网络框架**：如Netty的超时检测、连接管理
- **游戏服务器**：技能CD、Buff效果、定时刷新
- **定时任务调度**：周期性任务执行



第一步：实现单层轮，不带圈数的。

第二步：实现层级轮，带圈数的。



# 二、功能需求梳理

在正式写代码之前，我们先梳理下时间轮组件的基础组成：

- **数据结构定义有哪些？**
- **对外方法有哪些？**
- **内部方法有哪些？**

对外方法是使用这个时间轮组件的人进行调用的接口；

内部方法是具体的业务逻辑；

梳理清楚后，我们后续对照着每个节点去实现对应代码即可。

![img](https://gitee.com/codergeek/picgo-image/raw/master/image/202510141840763.jpeg)

这里有两个基于用Golang实现的基础：

**双向链表**：基于Golang的标准库container/list中已经实现的双向链表来进行定时任务的底层存储

**环状数据结构**：这里的环状数据结构其实并不需要我们进行实际意义上的收尾相连形成一个环，这里是基于数组，然后利用求余运算来逐个下标遍历方式，实现一个在逻辑上符合环状方式的目的。

![img](https://pica.zhimg.com/v2-c335f066c08fc2c01d7abce6e071907e_1440w.jpg)



# 三、代码实现

## 3、1 核心数据结构

### 1）TimeWheel时间轮结构体

```
type TimeWheel struct {
	interval     time.Duration
	ticker       *time.Ticker
	currentSlot  int
	slotNum      int
	slots        []*list.List
	stopCh       chan struct{}
	removeTaskCh chan string
	addTaskCh    chan *Task
	taskRecords  sync.Map
	mux          sync.Mutex
	isRun        bool
}
```

字段解释：

**currentSlot：**当前轮盘位置

**slots：**用于初始化轮盘数组内的元素 -- 链表（基于数组实现逻辑上的环状结构）

**taskRecords：**这是个sync.Map结构主要是将任务key和添加到链表的element进行映射和管理，方便后续任务删除等进行判断操作

**ticker：**定时器，一般根据interval设定触发时间间隔，利用channel传递定时到期通知

**stopChan:** 关闭时间轮的通道



而对于任务的管理主要是利用channel类型，主要是两个字段去处理：

**addTaskCh：**这个定时任务Task的核心结构，这个在后面会讲到

**removeTaskCh：**传递任务的唯一标记即可



### 2）Task任务结构体

```
type Task struct {
    //任务key 唯一
    key       string
    //执行的具体函数
    job       Job
    //任务执行时间
    executeAt time.Duration
    //执行次数 -1:重复执行
    times     int
    //轮盘位置
    slot      int
    //圈数
    circle    int
}
```

- **key：**任务的唯一ID，需要全局唯一
- **job：**定义的具体实际执行函数
- **executeAt：**任务的延迟执行时间
- **times：**执行次数，默认值是-1表示重复执行，如果是1表示执行一次
- **slot：**轮盘位置，可以通过它用下标的方式访问环形数组
- **circle：**圈数也可以说成轮次，如果添加的任务没有超过轮盘数，那么该值是0，如果超过：slotNum等于12添加15s执行的任务，那么circle = 1，依次类推。



Job是实际的执行函数，其函数定义为

```
type Job func(key string)
```

这是个入参为key的函数，可以在任务执行体中对key做一些校验。



## 3、2 核心函数方法

### 1）timewheel初始化

在使用时间轮之前需要先进行初始化，这里初始化提供两个参数：

**interval：**轮盘之间每个槽位的时间间隔

**slotNum：**轮盘的槽位数

```
func NewXTimingWheel(interval time.Duration, slotNum int) (*TimeWheel, error) {
        //参数判断
	if interval <= 0 {
		return nil, errors.New("minimum interval need one second")
	}
	if slotNum <= 0 {
		return nil, errors.New("minimum slotNum need greater than zero")
	}
	t := &TimeWheel{
		interval:     interval,
		currentSlot:  0,
		slotNum:      slotNum,
		slots:        make([]*list.List, slotNum),
		stopCh:       make(chan struct{}),
		removeTaskCh: make(chan string),
		addTaskCh:    make(chan *Task),
		isRun:        false,
	}
	t.start()
	return t, nil
}
```

start方法

```go
func (t *TimeWheel) start() {
        //判断时间轮时间在运行
	if !t.isRun {
                //根据slotNum初始化数组结构双向链表list
		for i := 0; i < t.slotNum; i++ {
			t.slots[i] = list.New()
		}
                //设置定时器时间间隔
		t.ticker = time.NewTicker(t.interval)
		t.mux.Lock()
		t.isRun = true
                //开启协程执行
		go t.run()
		t.mux.Unlock()
	}
}
```



### 2）任务监听和分发

for + select 多路复用方式监听多个 channel 的读写操作，从而实现新增任务、删除任务、停止时间轮、定时器到期等消息的传递和分发

![img](https://pic2.zhimg.com/v2-8faef5029ed4ff95cb581c78a388ea23_1440w.jpg)

```
func (t *TimeWheel) run() {
	for {
	select {
                //关闭时间轮， 退出本函数
		case <-t.stopCh:
			return
                //添加任务
		case task := <-t.addTaskCh:
			t.addTask(task)
                //删除任务
		case key := <-t.removeTaskCh:
			t.removeTask(key)
                //定时器信号
		case <-t.ticker.C:
			t.execute()
		}
	}
}
```



### 3）添加任务

我们梳理先添加任务的流程

- 对外方法AddTask，这里主要进行参数验证，然后将Task任务写入addTaskCh
- 时间轮处理channel中的添加任务事件
- 执行内部方法addTask来添加任务，涉及slot和circle计算和将任务追加到指定slot的双向链表list中

![img](https://pica.zhimg.com/v2-4f2bfcef8edcdc7733536bdb59f66a00_1440w.jpg)

```
func (t *TimeWheel) AddTask(key string, job Job, executeAt time.Duration, times int) error {
	if key == "" {
		return errors.New("key is empty")
	}
	if executeAt < t.interval {
		return errors.New("key is empty")
	}
        //sync.Map判断是否已添加过任务key
	_, ok := t.taskRecords.Load(key)
	if ok {
		return errors.New("key of job already exists")
	}
	task := &Task{
		key:       key,
		job:       job,
		times:     times,
		executeAt: executeAt,
	}
  //写入addTaskCh这个channel
	t.addTaskCh <- task
	return nil
}
```



内部关于AddTask的实现如下

```
func (t *TimeWheel) addTask(task *Task) {
        //计算slot和cirle的值
        slot, circle := t.calSlotAndCircle(task.executeAt)
        task.slot = slot
        task.circle = circle
        //追加到指定slot下标的list链表汇总
        ele := t.slots[slot].PushBack(task)
        //sync.Map保存key和链表中具体的任务
        t.taskRecords.Store(task.key, ele)
}
```



### 4）计算slot和circle

时间轮每次任务添加之前都会先进行slot和circle的计算，为了确定任务的在轮次中的位置和圈数，这两个参数在任务调度的时候很重要，会基于当前位置currentSlot的双向链表进行轮次循环等操作

```
func (t *TimeWheel) calSlotAndCircle(executeAt time.Duration) (slot, circle int) {
        //延迟时间 秒
        delay := int(executeAt.Seconds())
        //当前轮盘表示的时间 秒
        circleTime := len(t.slots) * int(t.interval.Seconds())
        //计算圈数
        circle = delay / circleTime
        //计算延迟时间对应的slot步长
        steps := delay / int(t.interval.Seconds())
        //计算位置slot
        slot = (t.currentSlot + steps) % len(t.slots)
        return
}
```



### 5）执行任务

通过前面初始化、计算slot和circle、任务添加之后我们的任务就已经添加到链表上了，等定时器触发到了任务所在slot后就到了执行阶段了，这里就是本文最关键的业务逻辑 --【执行任务】。

```
func (t *TimeWheel) execute() {
        //取出当前slot下标对应的 list
	taskList := t.slots[t.currentSlot]
        //判断list是否为空
	if taskList != nil {
                //遍历list
		for ele := taskList.Front(); ele != nil; {
                    //获取链表元素的定时任务
                    taskEle, _ := ele.Value.(*Task)
                    //判断任务circle (circle == 0才执行)
                    if taskEle.circle > 0 {
			taskEle.circle--
                         //返回链表的下一个元素
			ele = ele.Next()
			continue
			}
                    //执行任务函数
                    go taskEle.job(taskEle.key)
            
                    //删除key映射删除
                    t.taskRecords.Delete(taskEle.key)
                    //删除任务所在链表中元素
                    taskList.Remove(ele)
            
                    //执行固定次数任务
                    if taskEle.times-1 > 0 {
			taskEle.times--
			t.addTask(taskEle)
                    }
                    //重复任务
                    if taskEle.times == -1 {
                        t.addTask(taskEle)
                    }
                    ele = ele.Next()
		}
	}
        //将当前位置slot往前自增
	t.incrCurrentSlot()
}
```

流程总结如下：

1. 每当接收到ticker定时器信号时，会根据currentSlot，通过数组下标方式获取slots的任务list

2. 如果currentSlot的list部位空，那么进行遍历 list，从而获取 list 中的元素--待执行定时任务

3. 获取到待执行任务，会对任务中的参数 circle进行校验

   - circle > 0的任务，还未到达执行时间，需要将该circle 减1，等后续轮次再处理

   - circle = 0的任务，在当前轮次可被执行，开启一个协程，执行任务对应的函数

1. 任务执行后删除当前任务
2. 当任务的times参数大于0时，每执行一次，times减1，而times = -1的任务为重复执行任务
3. 当前currentSlot的任务遍历执行完毕，进入下一个轮盘位置
4. 等待下次ticker定时信号触发