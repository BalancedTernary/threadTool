#pragma once
#ifndef __MINE_MATH__
#define  __MINE_MATH__
#include "cmath"
#include <typeinfo>

#undef min
#undef max
namespace multMath
{
	template<typename _Tp1, typename _Tp2>
	inline auto min(const _Tp1& a, const _Tp2& b) noexcept -> const decltype(a < b ? a : b)
	{
		if (a > b)
		{
			return b;
		}
		else
		{
			return a;
		}
	}

	template<typename _Tp1, typename... _Args>
	inline const auto min(const _Tp1& a, const _Args&... arg) noexcept
	{
		return min(a, min(arg...));
	}


	template<typename _Tp1, typename _Tp2>
	inline auto max(const _Tp1& a, const  _Tp2& b) noexcept -> const decltype(a > b ? a : b)
	{
		if (a < b)
		{
			return b;
		}
		else
		{
			return a;
		}
	}

	template<typename _Tp1, typename... _Args>
	inline const auto max(const _Tp1& a, const _Args&... arg) noexcept
	{
		return max(a, max(arg...));
	}


	template<typename _Tp1>
	inline const _Tp1 abs(const _Tp1& a) noexcept
	{
		if (a < 0)
		{
			return std::move(-a);
		}
		else
		{
			return a;
		}
	}


	template<typename _Tp1>
	inline const auto sign(const _Tp1& a) noexcept
	{
		return std::move(_Tp1(a > 0 ? 1 : (a < 0 ? -1 : 0)));
	}

}


#endif
