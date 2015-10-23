#include "StdAfx.h"
#include "SevenThread.h"
#include "SevenWorkerPool.h"


namespace SevenZip
{

#define FOR_EACH_THREAD0(method) for (unsigned int _ = 0; _ < m_poolSize; ++_){m_threads[_]->##method();}

SevenWorkerPool::~SevenWorkerPool(void)
{
	FOR_EACH_THREAD0(Destroy)
	Join();

	::DeleteCriticalSection(&m_csObj);

}

SevenWorkerPool::SevenWorkerPool(void)
{
	::InitializeCriticalSectionAndSpinCount(&m_csObj, 5000);
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	m_poolSize = si.dwNumberOfProcessors * 2 + 2;
}

void SevenWorkerPool::Init()
{
	for (unsigned int i = 0; i < m_poolSize; ++i)
	{
		auto task = std::make_shared<SevenThread>(this);
		m_threads.push_back(task);
	}
}

void SevenWorkerPool::Start()
{
	FOR_EACH_THREAD0(Start);
}

void SevenWorkerPool::Stop()
{
	FOR_EACH_THREAD0(TaskDone);
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


std::function<void()> SevenWorkerPool::getTask()
{
	AUTO_SCOPE_LOCK();
	if (m_taskList.empty())
	{
		Stop();
		return std::function<void()>();
	}
	auto task = m_taskList.front();
	m_taskList.pop();
	return task;
}


void SevenWorkerPool::Join()
{
	FOR_EACH_THREAD0(Join);
}

void SevenWorkerPool::SetPoolSize(unsigned int size)
{
	m_poolSize = size;
}

void SevenWorkerPool::SubmitTask(const std::function<void()>& task)
{
	m_taskList.push(task);
}

void SevenWorkerPool::WaitDone()
{
	FOR_EACH_THREAD0(WaitDone);
}

unsigned int SevenWorkerPool::GetPoolSize() const
{
	return m_poolSize;
}

bool SevenWorkerPool::IsWorking()
{
	for (unsigned int i = 0; i < m_poolSize; ++i)
	{
		if (m_threads[i]->IsWorking())
		{
			return true;
		}
	}
	return false;
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