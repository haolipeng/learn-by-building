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
#include <mutex>
#include <cstdint>

/*全局函数声明*/
void *tickStepThreadGlobal(void* param);

class copyable
{
	
};

// 前向声明
class Entry;
typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> weakEntryPtr;

// TCP会话统计信息
struct SessionStats {
    uint64_t upBytes;      // 上行字节数
    uint64_t downBytes;    // 下行字节数
    uint64_t upPackets;    // 上行数据包数
    uint64_t downPackets;  // 下行数据包数

    SessionStats() : upBytes(0), downBytes(0), upPackets(0), downPackets(0) {}

    void updateUplink(uint64_t bytes, uint64_t packets = 1) {
        upBytes += bytes;
        upPackets += packets;
    }

    void updateDownlink(uint64_t bytes, uint64_t packets = 1) {
        downBytes += bytes;
        downPackets += packets;
    }
};

// TCP会话Key类（五元组）
class Sessionkey {
public:
    std::string dstIp;
    std::string srcIp;
    int dstPort;
    int srcPort;
    uint8_t protocol;  // 协议类型（TCP=6）
    weakEntryPtr context; // 存储弱引用
    SessionStats stats;   // 会话统计信息

    Sessionkey(const std::string& dst, const std::string& src, int dport, int sport, uint8_t proto = 6)
        : dstIp(dst), srcIp(src), dstPort(dport), srcPort(sport), protocol(proto) {}

    // 用于map的key比较（不包含统计信息）
    bool operator<(const Sessionkey& other) const {
        if (protocol != other.protocol) return protocol < other.protocol;
        if (dstIp != other.dstIp) return dstIp < other.dstIp;
        if (srcIp != other.srcIp) return srcIp < other.srcIp;
        if (dstPort != other.dstPort) return dstPort < other.dstPort;
        return srcPort < other.srcPort;
    }

    bool operator==(const Sessionkey& other) const {
        return protocol == other.protocol && dstIp == other.dstIp && srcIp == other.srcIp &&
               dstPort == other.dstPort && srcPort == other.srcPort;
    }

    // 设置上下文
    void setContext(const weakEntryPtr& ctx) {
        context = ctx;
    }

    // 更新统计信息
    void updateStats(bool isUplink, uint64_t bytes, uint64_t packets = 1) {
        if (isUplink) {
            stats.updateUplink(bytes, packets);
        } else {
            stats.updateDownlink(bytes, packets);
        }
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
	explicit Entry(const sessionkeyPtr& Key, int bucketIdx = 0)
		:sharedKey(Key), bucketIndex(bucketIdx)
	{

	}

	/*删除元素*/
	~Entry()
	{
		std::cout <<"use_count is "<<sharedKey.use_count();
		if(sharedKey.use_count()>0)
		{
			std::cout <<" element timeout! index is "<<timeoutNum;
			std::cout <<" Stats: up="<<sharedKey->stats.upBytes<<"B/"<<sharedKey->stats.upPackets<<"pkts";
			std::cout <<" down="<<sharedKey->stats.downBytes<<"B/"<<sharedKey->stats.downPackets<<"pkts"<<std::endl;

			Sessionkey* pKey = sharedKey.get();

			//从keyMap删除元素
			keyMap.erase(*pKey);

			timeoutNum++;
		}
	}

	sessionkeyPtr sharedKey;
	int bucketIndex;  // 记录当前所在的bucket索引，避免遍历所有bucket
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
	std::mutex mtx;  // 保护sessionKeyBuckets和keyMap的互斥锁

public:
	/* 检查元素是否存在 */
	bool checkElementExit(const Sessionkey& keyPtr);

	/*添加元素*/
	bool AddElement(const Sessionkey& keyPtr);

	/*更新会话：接收到数据后更新生命周期和统计信息*/
	bool UpdateSession(const Sessionkey& key, bool isUplink, uint64_t bytes, uint64_t packets = 1);

	/*获取会话统计信息*/
	bool GetSessionStats(const Sessionkey& key, SessionStats& stats);

	/*存储定时器队列*/
	void *timeoutSessionQueue;

	static int state;

private:
	/*内部辅助函数：移动entry到最新bucket*/
	void moveEntryToLatestBucket(EntryPtr& entry, int currentBucketIdx);

public:
	/*定时器线程*/
	void tickStepRun();

	/*打印定时器变化*/
	void dumpSessionKeyBuckets();
};



#endif