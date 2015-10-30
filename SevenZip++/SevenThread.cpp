#include "StdAfx.h"
#include "SevenWorkerPool.h"
#include "SevenThread.h"


namespace SevenZip
{
SevenThread::SevenThread(SevenWorkerPool* pool)
{
	m_hthread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &m_id);
	m_pool = pool;
	m_stop = false;
}


SevenThread::~SevenThread(void)
{
	Destroy();
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

void SevenThread::Destroy(DWORD timeout)
{
	m_stop = true;
	timeout = timeout>0 ? timeout : INFINITE;
	if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hthread, timeout))
	{
		::SuspendThread(m_hthread);
		::TerminateThread(m_hthread, -1);
	}
	::CloseHandle(m_hthread);
	m_hthread = nullptr;
}


void SevenThread::run()
{
	while (!m_stop)
	{
		auto task = m_pool->getTask();
		if (m_stop)
		{
			break;
		}
		if (task)
		{
			task.Task();
			if (task.Notify)
			{
				task.Notify();
			}
		}
	}
}

DWORD SevenThread::GetThreadId() const
{
	return m_id;
}


HANDLE SevenThread::GetThreadHandle() const
{
	return m_hthread;
}

}