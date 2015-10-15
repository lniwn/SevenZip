#include "StdAfx.h"
#include "SevenWorkerPool.h"
#include "SevenThread.h"


namespace SevenZip
{
SevenThread::SevenThread(SevenWorkerPool* pool)
{
	m_hthread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &m_id);
	m_workEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_doneEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_pool = pool;
	m_stop = false;
	m_alive = false;
	m_needWorking = false;
}


SevenThread::~SevenThread(void)
{
	Destroy();
	Join();
	::CloseHandle(m_hthread);
	m_hthread = nullptr;

	::CloseHandle(m_workEvent);
	m_workEvent = nullptr;

	::CloseHandle(m_doneEvent);
	m_doneEvent = nullptr;
}

DWORD WINAPI SevenThread::ThreadProc(_In_ LPVOID lpParameter)
{
	SevenThread* pthis = reinterpret_cast<SevenThread*>(lpParameter);
	pthis->run();
	return 0;
}


void SevenThread::Join()
{
	::WaitForSingleObject(m_hthread, INFINITE);
}

void SevenThread::Destroy()
{
	m_stop = true;
	m_needWorking = false;
	::SetEvent(m_workEvent);
}


void SevenThread::run()
{
	m_alive = true;
	while (true)
	{
		::WaitForSingleObject(m_workEvent, INFINITE);
		if (m_stop)
		{
			break;
		}
		while (m_needWorking)
		{
			auto task = m_pool->getTask();
			if (task)
			{
				task();
			}
		}
		::SetEvent(m_doneEvent);
	}
	::SetEvent(m_doneEvent);
	m_alive = false;
	m_stop = false;
}


bool SevenThread::IsAlive() const
{
	return m_alive;
}


DWORD SevenThread::GetThreadId() const
{
	return m_id;
}

void SevenThread::TaskDone()
{
	::ResetEvent(m_workEvent);
	m_needWorking = false;
}


void SevenThread::WaitDone()
{
	::WaitForSingleObject(m_doneEvent, INFINITE);
}

void SevenThread::Start()
{
	m_needWorking = true;
	::SetEvent(m_workEvent);
	::ResetEvent(m_doneEvent);
}


HANDLE SevenThread::GetThreadHandle() const
{
	return m_hthread;
}

}