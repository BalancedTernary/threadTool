#pragma once
#ifndef __ATOMIC__
#define __ATOMIC__
namespace threadTool
{
	template<typename _T>
	using Atomic = volatile std::atomic<_T>;
	template<typename _T>
	using _Reference = volatile _T&;
	template<typename _T>
	using AtomicReference = volatile _Reference<Atomic<_T>>;
	template<typename _T>
	using _ConstReference = volatile const _T&;
	template<typename _T>
	using AtomicConstReference = volatile _ConstReference<Atomic<_T>>;
};


#endif
