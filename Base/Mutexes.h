#pragma once

#include <mutex>

template<class T_> class MutexProtected_
{
	T_ val_;
	std::mutex mutex_;

public:
	template<typename... Args> 
	MutexProtected_(Args&&... args)
		:
	val_(std::forward<Args>(args)...)
	{	}

	// do whatever the caller asks, but in our scope
	template<class FUNC_, typename... Args>
	auto Apply(FUNC_ func, Args&&... args)
	{
		std::lock_guard<std::mutex> _(mutex_);
		return func(val_, std::forward<Args>(args)...);
	}
};

