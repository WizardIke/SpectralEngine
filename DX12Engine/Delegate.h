#pragma once
#include <type_traits>
#include <utility>

template<class T>
class Delegate;

template<class R, class... Args>
class Delegate<R(Args...)>
{
	void* context;
	R (*function)(void* context, Args... args);
public:
	Delegate() noexcept {}
		
	Delegate(R (*function)(void* context, Args... args)) noexcept :
		function(function) {}
		
	Delegate(void* context, R (*function)(void* context, Args... args)) noexcept :
		function(function), context(context) {}
	
	Delegate(const Delegate& other) noexcept = default;
		
	Delegate& operator=(const Delegate& other) noexcept = default;

	bool operator==(const Delegate& other) const noexcept
	{
		return context == other.context && function == other.function;
	}

	bool operator!=(const Delegate& other) const noexcept
	{
		return context != other.context || function != other.function;
	}
		
	template<class C, R (C::*memberFunction)(Args... args)>
	static Delegate make(C* c) noexcept
	{
		return Delegate(c, [](void* context, Args... args)
		{
			C* c = static_cast<C*>(context);
			return c->*memberFunction(args...);
		});
	}
	
	template<R (*freeFunction)(Args... args)>
	static Delegate make() noexcept
	{
		return Delegate([](void* context, Args... args)
		{
			return freeFunction(args...);
		});
	}
	
	template<class C, R (*freeFunction)(C* context, Args... args)>
	static Delegate make(C* c) noexcept
	{
		return Delegate(c, [](void* context, Args... args)
		{
			C* c = static_cast<C*>(context);
			return freeFunction(c, args...);
		});
	}

	R operator()(const Args&... args) const
	{
		return function(context, args...);
	}
};