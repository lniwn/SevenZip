#pragma once
#include <functional>

namespace SevenZip
{

class SevenWorkerPool;
class SevenThread:public NonCopyable
{
public:
	SevenThread(SevenWorkerPool* pool);
	~SevenThread(void);

	void Join(); ///< be careful, you may only need call me in the destructor
	void Destroy();
	void WaitDone();
	bool IsWorking() const;
	DWORD GetThreadId() const;
	void TaskDone();
	void Start();
	HANDLE GetThreadHandle() const;

private:
	static DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter);
	void run();

private:
	HANDLE m_hthread;
	HANDLE m_workEvent;
	HANDLE m_doneEvent;
	DWORD m_id;
	bool m_stop;
	bool m_alive;
	bool m_needWorking;
	SevenWorkerPool* m_pool;

};

}