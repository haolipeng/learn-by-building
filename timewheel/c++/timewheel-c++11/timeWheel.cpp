#include "timeWheel.h"

int CTimeWheel::state = 0;

void *tickStepThreadGlobal(void* param)
{
	CTimeWheel* pThis = (CTimeWheel*)param;
	pThis->tickStepRun();

	return NULL;
}

void CTimeWheel::tickStepRun()
{
	while(1)
	{
		{
			std::lock_guard<std::mutex> lock(mtx);
			sessionKeyBuckets.push_back(Bucket());
		}

		sleep(1);

		//VLOG(2)<<"tick step add 1";

		//当前map中剩余的元素
		//VLOG(2) << "remain size: " << keyMap.size() << std::endl;
	}
}


void CTimeWheel::dumpSessionKeyBuckets()
{
	int idx = 0;
	for (weakSessionKeyList::const_iterator bucketI = sessionKeyBuckets.begin();bucketI != sessionKeyBuckets.end();++bucketI, ++idx)
	{
		const Bucket& bucket = *bucketI;
		std::cout <<"index: "<<idx<<"  bucket set size is = "<<bucket.size() << std::endl;
		
		for (Bucket::const_iterator it = bucket.begin();it != bucket.end();++it)
		{
			//std::cout <<"raw pointer: "<<get_pointer(*it)<<" || "<<"use_count: "<<it->use_count() << std::endl;
			//std::cout <<"unordered_set size is "<<(*it).size()<<endl;
		}
	}
}


CTimeWheel::CTimeWheel(int idleSeconds, void* timeoutQueue)
{
	timeoutSessionQueue = timeoutQueue;

	sessionKeyBuckets.resize(idleSeconds);

	/*创建线程*/
	if(pthread_create(&tickThread,NULL,tickStepThreadGlobal,this)!=0)
	{
		std::cout <<"create tickStepThreadGlobal thread failed!" << std::endl;
	}
}

CTimeWheel::CTimeWheel()
{
	timeoutSessionQueue = NULL;
	sessionKeyBuckets.resize(10); // 默认10秒

	/*创建线程*/
	if(pthread_create(&tickThread,NULL,tickStepThreadGlobal,this)!=0)
	{
		std::cout <<"create tickStepThreadGlobal thread failed!" << std::endl;
	}
}

CTimeWheel::~CTimeWheel()
{
	pthread_join(tickThread,NULL);
}


/*
*检查元素在map中是否存在
*查找成功则返回true,失败则返回false
*查找成功,返回true
*/

bool CTimeWheel::checkElementExit(const Sessionkey& key)
{
	MapIterType ite;
	Sessionkey reverKey(key.dstIp,key.srcIp,
						key.dstPort,key.srcPort, key.protocol);

	//正向查找
	ite = keyMap.find(key);
	if(ite == keyMap.end())
	{
		//反向查找
		ite = keyMap.find(reverKey);
		if(ite == keyMap.end())
		{
			//元素第一次加入
			return false;
		}
	}

	Sessionkey rawkey = ite->first;

	// 如果找到元素，将其移动到最新的bucket中
	weakEntryPtr weakEntry = rawkey.context;
	EntryPtr entry = weakEntry.lock();
	if(entry)
	{
		moveEntryToLatestBucket(entry, entry->bucketIndex);
	}

	return true;
}

bool CTimeWheel::AddElement(const Sessionkey& rawKey)
{
	std::lock_guard<std::mutex> lock(mtx);

	//如果元素已存在，则更新map中元素
	if(true == checkElementExit(rawKey))
	{
		return false;
	}

	sessionkeyPtr sharedEntryPtr(new Sessionkey(rawKey));

	int currentBucketIdx = sessionKeyBuckets.size() - 1;
	EntryPtr entry(new Entry(sharedEntryPtr, currentBucketIdx));

	//将entry添加到当前时间轮尾部的bucket中
	sessionKeyBuckets.back().insert(entry);

	// 创建弱引用并设置到key中
	weakEntryPtr weakEntry(entry);
	sharedEntryPtr->setContext(weakEntry);

	// 使用shared_ptr作为key
	keyMap.insert(std::pair<Sessionkey,int>(*sharedEntryPtr,100));

	return true;
}

// 移动entry到最新的bucket（优化版，避免遍历所有bucket）
void CTimeWheel::moveEntryToLatestBucket(EntryPtr& entry, int currentBucketIdx)
{
	if (currentBucketIdx >= 0 && currentBucketIdx < sessionKeyBuckets.size())
	{
		// 从当前bucket中移除
		sessionKeyBuckets[currentBucketIdx].erase(entry);
	}

	// 添加到最新的bucket中
	int newBucketIdx = sessionKeyBuckets.size() - 1;
	sessionKeyBuckets.back().insert(entry);
	entry->bucketIndex = newBucketIdx;
}

// 更新会话：接收到数据后更新生命周期和统计信息
bool CTimeWheel::UpdateSession(const Sessionkey& key, bool isUplink, uint64_t bytes, uint64_t packets)
{
	std::lock_guard<std::mutex> lock(mtx);

	MapIterType ite;
	Sessionkey reverKey(key.dstIp, key.srcIp, key.dstPort, key.srcPort, key.protocol);

	//正向查找
	ite = keyMap.find(key);
	if(ite == keyMap.end())
	{
		//反向查找
		ite = keyMap.find(reverKey);
		if(ite == keyMap.end())
		{
			// 元素不存在，先添加
			Sessionkey newKey = key;
			newKey.updateStats(isUplink, bytes, packets);
			return AddElement(newKey);
		}
	}

	Sessionkey rawkey = ite->first;

	// 更新统计信息
	weakEntryPtr weakEntry = rawkey.context;
	EntryPtr entry = weakEntry.lock();
	if(entry)
	{
		entry->sharedKey->updateStats(isUplink, bytes, packets);
		// 移动到最新的bucket，刷新生命周期
		moveEntryToLatestBucket(entry, entry->bucketIndex);
		return true;
	}

	return false;
}

// 获取会话统计信息
bool CTimeWheel::GetSessionStats(const Sessionkey& key, SessionStats& stats)
{
	std::lock_guard<std::mutex> lock(mtx);

	MapIterType ite;
	Sessionkey reverKey(key.dstIp, key.srcIp, key.dstPort, key.srcPort, key.protocol);

	//正向查找
	ite = keyMap.find(key);
	if(ite == keyMap.end())
	{
		//反向查找
		ite = keyMap.find(reverKey);
		if(ite == keyMap.end())
		{
			return false;
		}
	}

	Sessionkey rawkey = ite->first;
	weakEntryPtr weakEntry = rawkey.context;
	EntryPtr entry = weakEntry.lock();
	if(entry)
	{
		stats = entry->sharedKey->stats;
		return true;
	}

	return false;
}