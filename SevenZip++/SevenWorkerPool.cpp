#include "StdAfx.h"
#include "SevenThread.h"
#include "SevenWorkerPool.h"
#include <cassert>


namespace SevenZip
{

#define FOR_EACH_THREAD0(method) for (unsigned int _ = 0; _ < m_pool_size_; ++_)\
	{auto beg = m_workList.begin(); std::advance(beg, _); (*beg)->##method();}

SevenWorkerPool::~SevenWorkerPool(void)
{
	Uninit();
}

SevenWorkerPool::SevenWorkerPool(void)
{
}

bool SevenWorkerPool::Init(unsigned int size)
{
	::InitializeCriticalSectionAndSpinCount(&m_csObj, 4000);
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	m_pool_size_ = si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 2;

	m_hWorkingEvt = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hIdleEvt = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	setTerminate(false);

	if (size > 0)
	{
		m_pool_size_ = size;
	}

	for (unsigned int i = 0; i < m_pool_size_; ++i)
	{
		auto task = std::make_shared<SevenThread>(this);
		m_workList.push_back(task);
	}
	
	return true;
}

void SevenWorkerPool::Uninit()
{
	Terminate();
	FOR_EACH_THREAD0(Join);
	::CloseHandle(m_hWorkingEvt);
	::CloseHandle(m_hIdleEvt);
	::DeleteCriticalSection(&m_csObj);
}


void SevenWorkerPool::Terminate()
{
	{
		AUTO_SCOPE_LOCK();
		while(!m_taskList.empty())
		{
			m_taskList.pop();
		}
	}
	setTerminate(true);
	::SetEvent(m_hIdleEvt);
	::SetEvent(m_hWorkingEvt);
}


bool SevenWorkerPool::runTask()
{
	SevenTask task;
	{
		AUTO_SCOPE_LOCK();

		if (!m_taskList.empty())
		{
			task = m_taskList.front();
			m_taskList.pop();
		}
	}
	if (task)
	{
		task();
		if (task.Notify)
		{
			task.Notify();
		}
	}
	else
	{
		::SignalObjectAndWait(m_hIdleEvt, m_hWorkingEvt, INFINITE, FALSE);
	}
	
	return !needTerminate();
}

//void SevenWorkerPool::SetPoolSize(unsigned int size)
//{
//	m_poolSize = size;
//}

void SevenWorkerPool::SubmitTask(const std::function<void()>& task, const std::function<void()>& notify)
{
	AUTO_SCOPE_LOCK();
	m_taskList.push(SevenTask(task, notify));
	::SetEvent(m_hWorkingEvt);
}

unsigned int SevenWorkerPool::GetPoolSize() const
{
	return m_pool_size_;
}

bool SevenWorkerPool::IsWorking()
{
	{
		AUTO_SCOPE_LOCK();
		if (!m_taskList.empty())
		{
			return true;
		}
	}
	
	for (auto itFind = m_workList.begin(); itFind != m_workList.end(); ++itFind)
	{
		if ((*itFind)->GetThreadState() == SevenThread::THREAD_RUNNING)
		{
			return true;
		}
	}
	return false;
}


bool SevenWorkerPool::needTerminate()
{
	return 1 == ::InterlockedCompareExchange(&m_lTerminate, 1, 1);
}

void SevenWorkerPool::setTerminate(bool yes)
{
	::InterlockedExchange(&m_lTerminate, static_cast<LONG>(yes));
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