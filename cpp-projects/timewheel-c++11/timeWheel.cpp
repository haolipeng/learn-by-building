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
		sessionKeyBuckets.push_back(Bucket());

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
						key.dstPort,key.srcPort);
	
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
		// 从所有bucket中移除该entry
		for(auto& bucket : sessionKeyBuckets)
		{
			bucket.erase(entry);
		}
		// 添加到最新的bucket中
		sessionKeyBuckets.back().insert(entry);
	}

	return true;
}

bool CTimeWheel::AddElement(const Sessionkey& rawKey)
{
	//如果元素已存在，则更新map中元素
	if(true == checkElementExit(rawKey))
	{
		return false;
	}
	
	sessionkeyPtr sharedEntryPtr(new Sessionkey(rawKey));

	EntryPtr entry(new Entry(sharedEntryPtr));

	//将entry添加到当前时间轮尾部的bucket中
	sessionKeyBuckets.back().insert(entry);

	// 创建弱引用并设置到key中
	weakEntryPtr weakEntry(entry);
	sharedEntryPtr->setContext(weakEntry);

	// 使用shared_ptr作为key
	keyMap.insert(std::pair<Sessionkey,int>(*sharedEntryPtr,100));

	return true;
}