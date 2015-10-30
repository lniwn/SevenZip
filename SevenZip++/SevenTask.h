#pragma once
#include <functional>

namespace SevenZip {
class SevenTask
{
public:
	SevenTask()
	{
	}

	explicit SevenTask(const std::function<void()>& task)
	{
		Task = task;
	}

	SevenTask(const std::function<void()>& task, const std::function<void()>& notify)
	{
		Task = task;
		Notify = notify;
	}

	operator bool() const
	{
		return static_cast<bool>(Task);
	}

	void operator()()
	{
		Task();
	}

	std::function<void()> Task;
	std::function<void()> Notify;
};

} // namespace SevenZip