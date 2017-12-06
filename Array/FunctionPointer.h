#pragma once
#include <type_traits>

template <typename value_type>
class functionPointer;

template <typename ReturnType, typename... Args>
class functionPointer<ReturnType(Args...)>
{
	typedef unsigned char Byte;
	// a function pointer type for the type-erasure behavior
	// the char* parameter is actually cast from some functor type
	typedef ReturnType(*typeErasedInvokeType)(Byte*, Args...);

	// type-aware generic function for invoking functor
	template <typename Functor>
	static ReturnType typedInvoke(Functor* functor, Args... args)
	{
		return (*functor)(std::forward<Args>(args)...);
	}

	typeErasedInvokeType invokeFunctor;
	Byte* functor;
public:
	functionPointer() : invokeFunctor(nullptr), functor(nullptr) {}

	// construct from any functor type including non-member functions
	template <typename Functor>
	functionPointer(Functor& functor) : invokeFunctor(reinterpret_cast<typeErasedInvokeType>(typedInvoke<Functor>)),
		functor(reinterpret_cast<Byte*>(&functor)) {}

	// copy constructor
	functionPointer(functionPointer const& other) : invokeFunctor(other.invokeFunctor), functor(other.functor) {}

	~functionPointer() {}

	ReturnType operator()(Args... args)
	{
		return invokeFunctor(functor, std::forward<Args>(args)...);
	}
};