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
	void Destroy(DWORD timeout = 0);
	DWORD GetThreadId() const;
	HANDLE GetThreadHandle() const;

private:
	static DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter);
	void run();

private:
	HANDLE m_hthread;
	DWORD m_id;
	bool m_stop;
	SevenWorkerPool* m_pool;
};

}