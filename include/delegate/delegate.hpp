/*
 * delegate.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef DELEGATE_DELEGATE_HPP_
#define DELEGATE_DELEGATE_HPP_

#include <utility>
#include <memory>

/**
 * Simple storage of a callable object for free and member functions.
 *
 * Design intent is to do part of what std::function do but
 * without heap allocation, virtual function call and with minimal
 * memory footprint. (2 pointers).
 * Use case is embedded systems where previously raw function pointers
 * where used, using a void* pointer to point out a particular structure/object.
 *
 * The price is less generality (No functors) and a bit of type fiddling
 * with void pointers.
 *
 * Helper functions are provided to handle the following cases:
 * - Call a member function on a specific object.
 * - Call a free function of signature  R(*)(T*, Args...); (T is some arbitrary
 * type).
 * - Call a free function of signature void(*)(void*, Args...); (function can be
 * a function pointer)
 *
 * In the code storing the callback, construct an object of type 'delegate'.
 * This is a nullable type which can be copied freely, containing 2 pointers.
 *
 * User code assigns this object using 'assign' member functions.
 *
 * Call the delegate by regular operator(). E.g.
 * delegate cb = ...; // Set up some callback target.
 * ...
 * int val = 0;
 * cb(val);
 * For free functions, the supplied void pointer get the value from the call
 * setup.
 *
 * The null value for the delegate is a special nullFkn. Calling a delegate
 * with a null value is a no-op and the possibly returned value is
 * a default constructed object of the relevant type.
 */
template <typename T>
class delegate;

namespace details
{
template <typename T>
T
nullReturnFunction()
{
    return T{};
}
template <>
void
nullReturnFunction()
{
    return;
}

// Helper struct to make sure no temporary values are passed.
// Accept res and const ref but not r-value references.
template<typename T>
struct NonTemporaryRef;

template<typename T>
struct NonTemporaryRef<const T>
{
	NonTemporaryRef(const T& ref) : obj_p(&ref) {}
	const T* obj_p;
};

template<typename T>
struct NonTemporaryRef
{
	NonTemporaryRef(T& ref) : obj_p(&ref) {}
	NonTemporaryRef(T&& ref) = delete;

	T* obj_p;
};

}

/**
 * Class for storing the callable object
 * Stores a pointer to an adapter function and a void* pointer to the
 * Object.
 *
 * @param R type of the return value from calling the callback.
 * @param Args Argument list to the function when calling the callback.
 */
template <typename R, typename... Args>
class delegate<R(Args...)>
{
    // Adaptor function for when the delegate is expected to be a nullptr.
    inline static R doNullFkn(void* v, Args... args)
    {
        return details::nullReturnFunction<R>();
    }

    // Adaptor function for the case where void* is not forwarded
    // to the caller. (Just a normal function pointer.)
    template <R(freeFkn)(Args...)>
    inline static R doFreeCB(void* v, Args... args)
    {
        return freeFkn(args...);
    }

    // Adapter function for the member + object calling.
    template <class T, R (T::*memFkn)(Args...)>
    inline static R doMemberCB(void* o, Args... args)
    {
        T* obj = static_cast<T*>(o);
        return (((*obj).*(memFkn))(args...));
    }

    // Adapter function for the member + object calling.
    template <class T, R (T::*memFkn)(Args...) const>
    inline static R doConstMemberCB(void* o, Args... args)
    {
        T const* obj = static_cast<T*>(o);
        return (((*obj).*(memFkn))(args...));
    }

    // Adapter function for the free function with extra first arg
    // in the called function, set at callback construction.
    template <class Tptr, R(freeFknWithPtr)(Tptr, Args...)>
    inline static R doFreeCBWithPtr(void* o, Args... args)
    {
        Tptr obj = static_cast<Tptr>(o);
        return freeFknWithPtr(obj, args...);
    }

    // Adapter function for when the stored object is a pointer to a
    // callable object (stored elsewhere). Call it using operator().
    template <class Functor>
    inline static R doFunctor(void* o_arg, Args... args)
    {
        auto obj = static_cast<Functor*>(o_arg);
        return (*obj)(args...);
    }

    template <class Functor>
    inline static R doConstFunctor(void* o_arg, Args... args)
    {
        const Functor* obj = static_cast<Functor*>(o_arg);
        return (*obj)(args...);
    }

    // Type of the function pointer for the trampoline functions.
    using Trampoline = R (*)(void*, Args...);

    // Signature presented to the user when calling the callback.
    using TargetFreeCB = R (*)(Args...);

  public:
    // Default construct with stored ptr == nullptr.
    constexpr delegate(const std::nullptr_t& nptr = nullptr)
        : m_cb(&doNullFkn), m_ptr(nullptr){};

    ~delegate() = default;

    // Call the stored function. Requires: bool(*this) == true;
    // Will call trampoline fkn which will call the final fkn.
    constexpr R operator()(Args... args) const __attribute__((always_inline))
    {
        return m_cb(m_ptr, args...);
    }

    constexpr bool equal(const delegate& rhs) const
    {
        return m_cb == rhs.m_cb && m_ptr == rhs.m_ptr;
    }

    constexpr bool null() const
    {
        return m_cb == doNullFkn;
    }

    // Return true if a function pointer is stored.
    constexpr operator bool() const noexcept
    {
        return !null();
    }

    constexpr void clear()
    {
        m_cb = doNullFkn;
        m_ptr = nullptr;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    constexpr delegate& set()
    {
        if (fkn)
            m_cb = &doFreeCB<fkn>;
        else
            m_cb = &doNullFkn;
        m_ptr = nullptr;
        return *this;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    constexpr delegate& set(details::NonTemporaryRef<T> tr)
    {
        m_cb = &doMemberCB<T, memFkn>;
        m_ptr = static_cast<void*>(tr.obj_p);
        return *this;
    }

    template <class T, R (T::*memFkn)(Args... args) const>
    constexpr delegate& set(details::NonTemporaryRef<const T> tr)
    {
        m_cb = &doConstMemberCB<T, memFkn>;
        m_ptr = const_cast<void*>(static_cast<const void*>(tr.obj_p));
        return *this;
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    constexpr delegate& set(details::NonTemporaryRef<T> tr)
    {
        m_cb = &doFunctor<T>;
        m_ptr = static_cast<void*>(tr.obj_p);
        return *this;
    }

    template <class T>
    constexpr delegate& set(details::NonTemporaryRef<const T> tr)
    {
        m_cb = &doConstFunctor<T>;
        m_ptr = const_cast<void*>(static_cast<const void*>(tr.obj_p));
        return *this;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    static constexpr delegate make()
    {
    	delegate del;
    	return del.set<fkn>();
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args) const>
    static constexpr delegate make(const T& object)
    {
    	delegate del;
    	return del.set<const T, memFkn>(object);
    }

    template <class T, R (T::*memFkn)(Args... args)>
    static constexpr delegate make(T& object)
    {
    	delegate del;
    	return del.set<T, memFkn>(object);
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    static constexpr delegate make(T&& object)
    {
    	delegate del;
    	using T_ = std::remove_reference_t<T>;
        return del.set<T_>(details::NonTemporaryRef<T_>(object));
    }


    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <class Tptr, R (*fkn)(Tptr, Args... args)>
    static constexpr delegate makeFreeCBWithPtr(Tptr ptr)
    {
        auto cb = &doFreeCBWithPtr<Tptr, fkn>;
        return delegate(cb, static_cast<void*>(ptr));
    }

    /**
     * Create a callback to a free function with a void* pointer argument,
     * removing the need for an adapter function.
     */
    template <Trampoline fkn>
    static constexpr delegate makeVoidCB(void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

    /**
     * Create a callback using a run-time variable fkn pointer
     * using voids. No adapter function is used.
     */
    static constexpr delegate makeVoidCB(Trampoline fkn, void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

  private:
    // Create ordinary free function pointer callback.
    constexpr delegate(Trampoline cb, void* ptr) : m_cb(cb), m_ptr(ptr) {}

    Trampoline m_cb;
    void* m_ptr;
};

template <typename R, typename... Args>
bool
operator==(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs)
{
    return lhs.equal(rhs);
}

template <typename R, typename... Args>
bool
operator!=(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs)
{
    return !(lhs == rhs);
}

// Bite the bullet, this is how unique_ptr handle nullptr_t.
template <typename R, typename... Args>
bool
operator==(std::nullptr_t lhs, const delegate<R(Args...)>& rhs)
{
    return rhs.null();
}

template <typename R, typename... Args>
bool
operator!=(std::nullptr_t lhs, const delegate<R(Args...)>& rhs)
{
    return !(lhs == rhs);
}

template <typename R, typename... Args>
bool
operator==(const delegate<R(Args...)>& lhs, std::nullptr_t rhs)
{
    return lhs.null();
}

template <typename R, typename... Args>
bool
operator!=(const delegate<R(Args...)>& lhs, std::nullptr_t rhs)
{
    return !(lhs == rhs);
}

// No ordering operators ( operator< etc). This delegate represent several
// classes of pointers and is not a naturally ordered type.

/**
 * Helper macro to create a delegate for calling a member function.
 * Example of use:
 *
 * auto cb = MAKE_MEMBER_DEL(void(), SomeClass::memberFunction, obj);
 *
 * where 'obj' is of type 'SomeClass'.
 *
 * @param fknType Template parameter for the function signature in std::function
 * style.
 * @param memFknPtr address of member function pointer. C++ require full name
 * path.
 * @object object which the member function should be called on.
 */
#define MAKE_MEMBER_DEL(fknType, memFknPtr, object)                         \
                                                                            \
    (delegate<fknType>::make<std::remove_reference<decltype(object)>::type, \
                             memFkn>(object))

/**
 * Helper macro to create a delegate for calling a free function
 * or static class function.
 * Example of use:
 *
 * auto cb = MAKE_FREE_DEL(void(), fkn, ptr);
 *
 * where 'ptr' is a pointer to some type T and fkn a free
 * function.
 *
 * @param fknType Template parameter for the function signature in std::function
 * style.
 * @param fkn Free function to be called. signature: R (*)(T*, Args...);
 * @object Pointer to be supplied as first argument to function.
 *
 */
#define MAKE_FREE_DEL(fknType, fkn, ptr)                                       \
                                                                               \
    (delegate<fknType>::make<std::remove_reference<decltype(ptr)>::type, fkn>( \
        ptr))

#endif /* UTILITY_CALLBACK_H_ */
