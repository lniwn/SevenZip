#pragma once

namespace SevenZip
{

class NonCopyable
{
protected:
	NonCopyable(){}
	~NonCopyable(){}

private:
	NonCopyable(const NonCopyable&);
	NonCopyable(NonCopyable&&);
	NonCopyable& operator=(const NonCopyable&);
	NonCopyable& operator=(NonCopyable&&);
};
}