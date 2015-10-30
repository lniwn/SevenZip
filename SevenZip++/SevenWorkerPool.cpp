#include "StdAfx.h"
#include "SevenThread.h"
#include "SevenWorkerPool.h"
#include <cassert>


namespace SevenZip
{

#define FOR_EACH_THREAD0(method) for (unsigned int _ = 0; _ < m_poolSize; ++_){m_threads[_]->##method();}

SevenWorkerPool::~SevenWorkerPool(void)
{
	Uninit();
}

SevenWorkerPool::SevenWorkerPool(void)
{
	m_daemon = nullptr;
}

void SevenWorkerPool::Init(unsigned int size)
{
	if (!m_daemon)
	{
		::InitializeCriticalSectionAndSpinCount(&m_csObj, 5000);
		SYSTEM_INFO si;
		::GetSystemInfo(&si);
		m_stop = false;
		m_poolSize = si.dwNumberOfProcessors * 2 + 2;
		m_notify_done = nullptr;

		if (size > 0)
		{
			m_poolSize = size;
		}
		m_workingEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
		m_doneEvent = ::CreateEvent(nullptr, TRUE, TRUE, nullptr);
		m_daemon = ::CreateThread(nullptr, 0, DaemonThreadProc, this, 0, nullptr);
		::SetThreadPriority(m_daemon, THREAD_PRIORITY_BELOW_NORMAL);

		for (unsigned int i = 0; i < m_poolSize; ++i)
		{
			auto task = std::make_shared<SevenThread>(this);
			m_threads.push_back(task);
		}
	}
}

void SevenWorkerPool::Uninit()
{
	if (m_daemon)
	{
		m_stop = true;
		::SetEvent(m_workingEvent);
		FOR_EACH_THREAD0(Destroy)
		
		::SetEvent(m_doneEvent);
		::WaitForSingleObject(m_daemon, INFINITE);
		::CloseHandle(m_daemon);
		m_daemon = nullptr;

		::DeleteCriticalSection(&m_csObj);
		::CloseHandle(m_workingEvent);
		::CloseHandle(m_doneEvent);
	}
}

void SevenWorkerPool::Start()
{
	assert(m_daemon);
	if (!IsWorking())
	{
		for each(auto& t in m_threads)
		{
			::SetThreadPriority(t->GetThreadHandle(), THREAD_PRIORITY_NORMAL);
		}
		m_doneCount = 0;
		::ResetEvent(m_doneEvent);
		::SetEvent(m_workingEvent);
	}
	
}

void SevenWorkerPool::Stop()
{
	::ResetEvent(m_workingEvent);
}

bool SevenWorkerPool::ClearTasks()
{
	if (IsWorking())
	{
		return false;
	}
	while (!m_taskList.empty())
	{
		m_taskList.pop();
	}
	return true;
}


SevenTask SevenWorkerPool::getTask()
{
	::WaitForSingleObject(m_workingEvent, INFINITE);
	{
		AUTO_SCOPE_LOCK();
		auto cur_thread = ::GetCurrentThread();
		if (m_taskList.empty())
		{
			if (::GetThreadPriority(cur_thread) != THREAD_PRIORITY_IDLE)
			{
				if(m_poolSize <= ++m_doneCount)
				{
					::SetEvent(m_doneEvent);
					::ResetEvent(m_workingEvent);
				}
				::SetThreadPriority(cur_thread, THREAD_PRIORITY_IDLE);
			}
			::SwitchToThread();
			
			return SevenTask();
		}
		else
		{
			if (::GetThreadPriority(cur_thread) != THREAD_PRIORITY_NORMAL)
			{
				::SetThreadPriority(cur_thread, THREAD_PRIORITY_NORMAL);
				--m_doneCount;
			}
			auto task = m_taskList.front();
			m_taskList.pop();
			return task;
		}
	}
}


void SevenWorkerPool::Join()
{
	FOR_EACH_THREAD0(Join);
}

//void SevenWorkerPool::SetPoolSize(unsigned int size)
//{
//	m_poolSize = size;
//}

void SevenWorkerPool::SubmitTask(const std::function<void()>& task)
{
	m_taskList.push(SevenTask(task));
}

void SevenWorkerPool::SubmitTask(const std::function<void()>& task, const std::function<void()>& notify)
{
	m_taskList.push(SevenTask(task, notify));
}

bool SevenWorkerPool::WaitDone(DWORD timeout)
{
	timeout = timeout>0 ? timeout : INFINITE;
	return WAIT_TIMEOUT != ::WaitForSingleObject(m_doneEvent, timeout);
}

void SevenWorkerPool::Execute(const std::function<void()>& task, std::function<void()> notify)
{
	{
		AUTO_SCOPE_LOCK();
		SubmitTask(task, notify);
	}
	Start();
}

unsigned int SevenWorkerPool::GetPoolSize() const
{
	return m_poolSize;
}

bool SevenWorkerPool::IsWorking()
{
	return WAIT_TIMEOUT == ::WaitForSingleObject(m_doneEvent, 0);
}

void SevenWorkerPool::NotifyDone(const std::function<void()>& notify)
{
	m_notify_done = notify;
}

DWORD WINAPI SevenWorkerPool::DaemonThreadProc(_In_ LPVOID lpParameter)
{
	auto* pthis = reinterpret_cast<SevenWorkerPool*>(lpParameter);
	while (!pthis->m_stop)
	{
		::WaitForSingleObject(pthis->m_workingEvent, INFINITE);
		pthis->WaitDone();
		if (pthis->m_notify_done)
		{
			pthis->m_notify_done();
			pthis->m_notify_done = nullptr;
		}
	}
	return 0;
}


SimpleMemoryPool::SimpleMemoryPool()
{
	::InitializeCriticalSectionAndSpinCount(&m_csObj, 5000);
	const size_t memsize = 1024;
	for (unsigned int i = 0; i < 5; ++i)
	{
		put(memsize);
	}
}

SimpleMemoryPool::~SimpleMemoryPool()
{
	::DeleteCriticalSection(&m_csObj);
}

SimpleMemoryPool::MemTuple* SimpleMemoryPool::put(size_t size)
{
	AUTO_SCOPE_LOCK();
	m_pool.push_back(std::make_tuple(size, 
		std::shared_ptr<byte>(new byte[size], std::default_delete<byte[]>()), FALSE));
	return &m_pool.back();
}

void* SimpleMemoryPool::Get(size_t size)
{
	AUTO_SCOPE_LOCK();
	const auto& find_item = std::find_if(m_pool.begin(), m_pool.end(), 
		[&size](const std::tuple< size_t, std::shared_ptr<byte>, BOOL >& value)->bool
	{
		return !std::get<2>(value) && std::get<0>(value) >= size;
	});
	MemTuple* _item = nullptr;
	if (find_item == m_pool.end())
	{
		_item = put(size);
	}
	else
	{
		_item = &*find_item;
	}
	std::get<2>(*_item) = TRUE;
	return std::get<1>(*_item).get();
}

bool SimpleMemoryPool::Done(void* baseaddr)
{
	AUTO_SCOPE_LOCK();
	const auto& find_item = std::find_if(m_pool.begin(), m_pool.end(), 
		[&baseaddr](const std::tuple< size_t, std::shared_ptr<byte>, BOOL >& value)->bool
	{
		return std::get<1>(value).get() == baseaddr;
	});
	if (find_item != m_pool.end())
	{
		std::get<2>(*find_item) = FALSE;
		return true;
	}
	return false;
}

void SimpleMemoryPool::Shrink_to_fit()
{
	AUTO_SCOPE_LOCK();
	m_pool.remove_if([](const MemTuple& value)->bool
	{
		return !std::get<2>(value);
	});
}

} // namespace SevenZip