//
#pragma once

#include <tbb/atomic.h>

#if 1 // relaxed mem consistency model (~50% faster than MSC)
#define MemFullFence tbb::full_fence
#define MemRelaxed tbb::relaxed
#define MemAcquire tbb::acquire
#define MemRelease tbb::release
#else // memory sequential consistent 
#define MemFullFence tbb::full_fence
#define MemRelaxed tbb::full_fence
#define MemAcquire tbb::full_fence
#define MemRelease tbb::full_fence
#endif

namespace ork
{
	template <typename T> using atomic = tbb::atomic<T>;
};

#if ! defined(ORK_OSX)
namespace std
{
	template <typename T> using atomic = tbb::atomic<T>;
};
#endif