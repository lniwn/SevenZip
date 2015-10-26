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
#define AUTO_SCOPE_LOCK() AUTO_CRITICAL_SECTION _(&m_csObj)
friend class SevenThread;
public:
	SevenWorkerPool(void);
	~SevenWorkerPool(void);

	void Init(unsigned int size = 0);
	void Start();
	void Stop();
	void Join();
	void WaitDone();
	bool IsWorking();
	bool ClearTasks();

	///< notify all tasks have done
	void NotifyDone(const std::function<void()>& notify);

	template<typename I, typename N>
	void SubmitTasks(I begin, I end, N notifys)
	{
		while(begin != end)
		{
			m_taskList.push(SevenTask(*begin++, *notifys++));
		}
	}

	template<typename I>
	void SubmitTasks(I begin, I end)
	{
		while(begin != end)
		{
			m_taskList.push(*begin++);
		}
	}

	void SubmitTask(const std::function<void()>& task);
	void SubmitTask(const std::function<void()>& task, const std::function<void()>& notify);
	//void SetPoolSize(unsigned int size);
	unsigned int GetPoolSize() const;

private:
	SevenTask getTask();
	static DWORD WINAPI DaemonThreadProc(_In_ LPVOID lpParameter);
	
private:
	std::vector< std::shared_ptr<SevenThread> > m_threads;
	HANDLE m_daemon;
	bool m_stop;
	std::queue<SevenTask> m_taskList;
	CRITICAL_SECTION m_csObj;
	unsigned int m_poolSize;
	std::function<void()> m_notify_done;
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