#pragma once
#include <functional>

namespace SevenZip
{

class SevenWorkerPool;
class SevenThread:public NonCopyable
{
public:
	enum ThreadState
	{
		THREAD_CREATE, // 创建初期，尚未执行线程体
		THREAD_SUSPEND, // 线程被挂起
		THREAD_RUNNING, // 线程正在执行
		THREAD_DEAD // 线程被销毁
	};
	explicit SevenThread(SevenWorkerPool* pool);
	~SevenThread(void);

	void Join();
	void Destroy(DWORD timeout = 0);
	DWORD GetThreadId() const {return m_id;}
	HANDLE GetThreadHandle() const {return m_hthread;}
	ThreadState GetThreadState() {return static_cast<ThreadState>(::InterlockedCompareExchange(&m_state, m_state, m_state));}

private:
	static DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter);
	void run();

private:
	HANDLE m_hthread;
	DWORD m_id;
	SevenWorkerPool* m_pool;
	LONG m_state;
};

}