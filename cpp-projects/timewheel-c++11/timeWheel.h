#ifndef TIME_WHEEL_H
#define TIME_WHEEL_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <pthread.h>
#include <unistd.h>

/*全局函数声明*/
void *tickStepThreadGlobal(void* param);

class copyable
{
	
};

// 前向声明
class Entry;
typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> weakEntryPtr;

// 简化的Sessionkey类
class Sessionkey {
public:
    std::string dstIp;
    std::string srcIp;
    int dstPort;
    int srcPort;
    weakEntryPtr context; // 存储弱引用
    
    Sessionkey(const std::string& dst, const std::string& src, int dport, int sport)
        : dstIp(dst), srcIp(src), dstPort(dport), srcPort(sport) {}
    
    // 用于map的key比较
    bool operator<(const Sessionkey& other) const {
        if (dstIp != other.dstIp) return dstIp < other.dstIp;
        if (srcIp != other.srcIp) return srcIp < other.srcIp;
        if (dstPort != other.dstPort) return dstPort < other.dstPort;
        return srcPort < other.srcPort;
    }
    
    bool operator==(const Sessionkey& other) const {
        return dstIp == other.dstIp && srcIp == other.srcIp && 
               dstPort == other.dstPort && srcPort == other.srcPort;
    }
    
    // 设置上下文
    void setContext(const weakEntryPtr& ctx) {
        context = ctx;
    }
};

// 使用标准库的智能指针
typedef std::weak_ptr<Sessionkey> weakSessionKeyPtr;
typedef std::shared_ptr<Sessionkey> sessionkeyPtr;

typedef std::map<Sessionkey, int> ConnectionMap;
typedef std::map<Sessionkey, int>::iterator MapIterType;

static ConnectionMap keyMap;
static uint64_t timeoutNum = 0;

/*Entry结构体,使用shared_ptr*/
class Entry: public copyable
{
public:
	explicit Entry(const sessionkeyPtr& Key)
		:sharedKey(Key)
	{
		
	}

	/*删除元素*/
	~Entry()
	{
		std::cout <<"use_count is "<<sharedKey.use_count();
		if(sharedKey.use_count()>0)
		{	
			std::cout <<"element timeout! index is "<<timeoutNum;
			Sessionkey* pKey = sharedKey.get();

			//从keyMap删除元素
			keyMap.erase(*pKey);

			timeoutNum++;
		}
	}

	sessionkeyPtr sharedKey;
};


class CTimeWheel
{
public:
	CTimeWheel();
	~CTimeWheel();

	CTimeWheel(int idleSeconds, void* timeoutQueue);

public:
	typedef std::shared_ptr<Entry> EntryPtr;
	typedef std::weak_ptr<Entry> weakEntryPtr;
	
	typedef std::unordered_set<EntryPtr> Bucket;
	typedef std::vector<Bucket> weakSessionKeyList;

	weakSessionKeyList   sessionKeyBuckets;

	pthread_t tickThread;
	
public:
	/* 检查元素是否存在 */	
	bool checkElementExit(const Sessionkey& keyPtr);

	/*添加元素*/
	bool AddElement(const Sessionkey& keyPtr);
	
	/*存储定时器队列*/
	void *timeoutSessionQueue;
	
	static int state;

public:
	/*定时器线程*/
	void tickStepRun();


	/*打印定时器变化*/
	void dumpSessionKeyBuckets();
};



#endif