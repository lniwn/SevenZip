#pragma once
#include <functional>

namespace SevenZip {
class SevenTask
{
public:
	SevenTask()
	{
		Task = [](){};
		Notify = [](){};
	}

	explicit SevenTask(const std::function<void()>& task)
	{
		Task = task;
		Notify = [](){};
	}

	SevenTask(const std::function<void()>& task, const std::function<void()>& notify)
	{
		Task = task;
		Notify = notify;
	}

	std::function<void()> Task;
	std::function<void()> Notify;
};

} // namespace SevenZip