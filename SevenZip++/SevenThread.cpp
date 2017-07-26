#include "StdAfx.h"
#include "SevenWorkerPool.h"
#include "SevenThread.h"


namespace SevenZip
{
SevenThread::SevenThread(SevenWorkerPool* pool)
{
	m_hthread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, &m_id);
	m_state = THREAD_CREATE;
	m_pool = pool;
}


SevenThread::~SevenThread(void)
{
	Destroy();
}

DWORD WINAPI SevenThread::ThreadProc(_In_ LPVOID lpParameter)
{
	SevenThread* pthis = reinterpret_cast<SevenThread*>(lpParameter);
	::InterlockedExchange(&pthis->m_state, THREAD_RUNNING);
	pthis->run();
	::InterlockedExchange(&pthis->m_state, THREAD_DEAD);
	return 0;
}


void SevenThread::Join()
{
	::WaitForSingleObject(m_hthread, INFINITE);
}

void SevenThread::Destroy(DWORD timeout)
{
	if (m_hthread)
	{
		timeout = timeout>0 ? timeout : INFINITE;
		if (WAIT_TIMEOUT == ::WaitForSingleObject(m_hthread, timeout))
		{
			::SuspendThread(m_hthread);
			::InterlockedExchange(&m_state, THREAD_SUSPEND);
			::TerminateThread(m_hthread, -1);
		}
		::CloseHandle(m_hthread);
		m_hthread = nullptr;
		::InterlockedExchange(&m_state, THREAD_DEAD);
	}
}


void SevenThread::run()
{
	while (m_pool->runTask()){}
}

}