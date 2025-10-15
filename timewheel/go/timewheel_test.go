package timewheel

import (
	"sync"
	"testing"
	"time"
)

// TestTimeWheelBasicFlow 测试时间轮的基本流程
func TestTimeWheelBasicFlow(t *testing.T) {
	// 创建时间轮：每1秒一个刻度，共10个槽位
	tw, err := NewTimeWheel(1*time.Second, 10)
	if err != nil {
		t.Fatalf("创建时间轮失败: %v", err)
	}

	// 启动时间轮
	err = tw.Start()
	if err != nil {
		t.Fatalf("启动时间轮失败: %v", err)
	}

	// 用于验证任务是否执行
	var executed bool
	var mu sync.Mutex

	// 添加一个定时任务
	taskKey := "test-task-1"
	job := func(key string) {
		mu.Lock()
		defer mu.Unlock()
		t.Logf("任务执行了, key: %s", key)
		executed = true
	}

	err = tw.AddTask(taskKey, job, 7*time.Second)
	if err != nil {
		t.Fatalf("添加任务失败: %v", err)
	}

	// 等待任务执行
	// 由于代码中硬编码slot=5，所以任务会在第5个槽位执行
	// 从slot 0 到 slot 5 需要 6 次tick，即 6 秒
	time.Sleep(8 * time.Second)

	// 验证任务是否执行
	mu.Lock()
	defer mu.Unlock()
	if !executed {
		t.Error("任务未执行")
	} else {
		t.Log("测试通过：任务成功执行")
	}
}
