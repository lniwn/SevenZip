#pragma once
#include <functional>
#include <memory>
#include <queue>
#include <tuple>
#include <list>
#include "SevenTask.h"

namespace SevenZip
{
class AUTO_CRITICAL_SECTION
{
public:
	AUTO_CRITICAL_SECTION(CRITICAL_SECTION* pcs){::EnterCriticalSection(pcs); m_pcs = pcs;}
	~AUTO_CRITICAL_SECTION(){::LeaveCriticalSection(m_pcs);}

private:
	CRITICAL_SECTION* m_pcs;
};

class SevenWorkerPool:public NonCopyable
{
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define AUTO_SCOPE_LOCK() AUTO_CRITICAL_SECTION MACRO_CONCAT(ThreadSafe, __COUNTER__)(&m_csObj)

friend class SevenThread;
public:
	SevenWorkerPool(void);
	~SevenWorkerPool(void);

	bool Init(unsigned int size = 0);
	void Uninit();
	bool IsWorking();
	void Terminate();

	template<typename I>
	void SubmitTasks(I begin, I end, const std::function<void()>& notify)
	{
		//AUTO_SCOPE_LOCK();
		while(begin != end)
		{
			//m_taskList.push(SevenTask(*begin++, notify));
			SubmitTask(*begin++, notify);
		}
	}

	template<typename I>
	void SubmitTasks(I begin, I end)
	{
		//AUTO_SCOPE_LOCK();
		while(begin != end)
		{
			//m_taskList.push(SevenTask(*begin++));
			SubmitTask(*begin++);
		}
	}

	void SubmitTask(const std::function<void()>& task, const std::function<void()>& notify = nullptr);
	//void SetPoolSize(unsigned int size);
	unsigned int GetPoolSize() const;

private:
	bool runTask();
	static DWORD WINAPI DaemonThreadProc(_In_ LPVOID lpParameter);
	bool needTerminate();
	void setTerminate(bool yes);
	
private:
	std::queue<SevenTask> m_taskList; // 任务队列
	std::list<std::shared_ptr<SevenThread> > m_workList; // 工作线程队列
	CRITICAL_SECTION m_csObj;
	unsigned int m_pool_size_;
	HANDLE m_hWorkingEvt;
	HANDLE m_hIdleEvt;
	LONG m_lTerminate;
};

class SimpleMemoryPool:public NonCopyable
{
	typedef std::tuple< size_t, std::shared_ptr<byte>, BOOL > MemTuple;
public:
	SimpleMemoryPool();
	~SimpleMemoryPool();
	void* Get(size_t size);
	bool Done(void* baseaddr);
	void Shrink_to_fit();

private:
	MemTuple* put(size_t size);

private:
	std::list< MemTuple > m_pool;
	CRITICAL_SECTION m_csObj;
};

} // namespace SevenZip