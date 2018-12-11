/*
 * delegate.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef DELEGATE_DELEGATE_HPP_
#define DELEGATE_DELEGATE_HPP_

#include <cstddef> // nullptr_t

/**
 * Simple storage of a callable object for functors, free and member functions.
 *
 * Design intent is to do part of what std::function do but
 * without heap allocation, virtual function call and with minimal
 * memory footprint. (2 pointers).
 * Use case is embedded systems where previously raw function pointers
 * where used, using a void* pointer to point out a particular structure/object.
 *
 * The price is less generality. The user must keep objects alive since the
 * delegate only store a pointer to them. This is true for both
 * member functions and functors.
 *
 * Free functions and member functions are supplied as compile time template
 * arguments. It is required to generate the correct intermediate functions.
 *
 * Once constructed, the delegate should behave as any pointer:
 * - Can be copied freely.
 * - Can be compared to same type delegates and nullptr.
 * - Can be reassigned.
 * - Can be called.
 *
 * A default constructed delegate compare equal to nullptr. However it can be
 * called with the default behavior being to do nothing and return a default
 * constructed return object.
 *
 * Two overload sets are provided for construction:
 * - set : Set an existing delegate with new pointer values.
 * - make : Construct a new delegate.
 *
 * The following types of callables are supported:
 * - Functors.
 * - Member functions.
 * - Free functions. (Do not use the stored void* value)
 * - (Special) A free function with a void* extra first argument. That will
 *   be passed the void* value set at delegate construction,
 *   in addition to the arguments supplied to the call.
 *
 * Const correctness:
 * The delegate models a pointer in const correctness. The constness of the
 * delegate is different that the constness of the called objects.
 * Calling a member function or operator() require them to be const to be able
 * to call a const object.
 *
 * Both the member function and the object to call on must be set
 * at the same time. This is required to maintain const correctness guarantees.
 * The constness of the object or the member function is not part of the
 * delegate type.
 *
 * There is a class MemFkn for storing pointers to member functions which
 * will keep track of constness. It allow taking the address of a member
 * function at one point and store it. At a later time the MemFkn object and an
 * object can be set to a delegate for later call.
 *
 * The delegate do not allow storing pointers to r-value references
 * (temporary objects) for member and functor construction.
 */

#if __cplusplus < 201103L
#error "Require at least C++11 to compile delegate"
#endif

#if __cplusplus >= 201402L
#define DELEGATE_CXX14CONSTEXPR constexpr
#else
#define DELEGATE_CXX14CONSTEXPR
#endif

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
} // namespace details

template <typename T>
class delegate;

template <typename Del, bool cnst>
class MemFkn
{
  public:
    explicit constexpr MemFkn() = default;
    explicit constexpr MemFkn(typename Del::Trampoline f) : trampoline(f) {}

    constexpr bool null() const noexcept
    {
        return trampoline == Del::doNullFkn;
    }

    constexpr bool equal(const MemFkn& rhs) const noexcept
    {
        return trampoline == rhs.trampoline;
    }

    // Define a total order for purpose of sorting in maps etc.
    // Do not define operators since this is not a natural total order.
    // It will vary randomly depending on where symbols end up etc.
    constexpr bool less(const MemFkn& rhs) const noexcept
    {
        return (null() && !rhs.null()) || trampoline < rhs.trampoline;
    }

    // Return true if a function pointer is stored.
    constexpr operator bool() const noexcept
    {
        return !null();
    }

  private:
    friend Del;
    typename Del::Trampoline trampoline = Del::doNullFkn;
};

/**
 * Class for storing the callable object
 * Stores a pointer to an adapter function and a void* pointer to the
 * Object.
 *
 * @param R type of the return value from calling the callback.
 * @param Args Argument list to the function when calling the callback.
 */
template <typename R_, typename... Args>
class delegate<R_(Args...)>
{
    using R = R_;
    friend class MemFkn<delegate, false>;
    friend class MemFkn<delegate, true>;

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
    // in the called function, set at delegate construction.
    template <class T, R(freeFkn)(T&, Args...)>
    inline static R dofreeFknWithObjectRef(void* o, Args... args)
    {
        T* obj = static_cast<T*>(o);
        return freeFkn(*obj, args...);
    }

    // Adapter function for the free function with extra first arg
    // in the called function, set at delegate construction.
    template <class T, R(freeFkn)(T const&, Args...)>
    inline static R dofreeFknWithObjectConstRef(void* o, Args... args)
    {
        T const* obj = static_cast<T*>(o);
        return freeFkn(*obj, args...);
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
    constexpr delegate(const std::nullptr_t& nptr = nullptr) noexcept
        : m_cb(&doNullFkn), m_ptr(nullptr){};

    ~delegate() = default;

    // Call the stored function. Requires: bool(*this) == true;
    // Will call trampoline fkn which will call the final fkn.
    constexpr R operator()(Args... args) const __attribute__((always_inline))
    {
        return m_cb(m_ptr, args...);
    }

    constexpr bool null() const noexcept
    {
        return m_cb == doNullFkn;
    }

    constexpr bool equal(const delegate& rhs) const noexcept
    {
        return m_cb == rhs.m_cb && m_ptr == rhs.m_ptr;
    }

    // Define a total order for purpose of sorting in maps etc.
    // Do not define operators since this is not a natural total order.
    // It will vary randomly depending on where symbols end up etc.
    constexpr bool less(const delegate& rhs) const noexcept
    {
        return (null() && !rhs.null()) || m_cb < rhs.m_cb ||
               (m_cb == rhs.m_cb && m_ptr < rhs.m_ptr);
    }

    // Return true if a function pointer is stored.
    constexpr operator bool() const noexcept
    {
        return !null();
    }

    DELEGATE_CXX14CONSTEXPR void clear()
    {
        m_cb = doNullFkn;
        m_ptr = nullptr;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    DELEGATE_CXX14CONSTEXPR delegate& set()
    {
        m_cb = fkn ? &doFreeCB<fkn> : &doNullFkn;
        m_ptr = nullptr;
        return *this;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    DELEGATE_CXX14CONSTEXPR delegate& set(T& tr)
    {
        m_cb = &doMemberCB<T, memFkn>;
        m_ptr = static_cast<void*>(&tr);
        return *this;
    }

    template <class T, R (T::*memFkn)(Args... args) const>
    DELEGATE_CXX14CONSTEXPR delegate& set(T const& tr)
    {
        m_cb = &doConstMemberCB<T, memFkn>;
        m_ptr = const_cast<void*>(static_cast<const void*>(&tr));
        return *this;
    }

    // Delete r-values. Not interested in temporaries.
    template <class T, R (T::*memFkn)(Args... args)>
    DELEGATE_CXX14CONSTEXPR delegate& set(T&&) = delete;

    template <class T, R (T::*memFkn)(Args... args) const>
    DELEGATE_CXX14CONSTEXPR delegate& set(T&&) = delete;

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(T& tr)
    {
        m_cb = &doFunctor<T>;
        m_ptr = static_cast<void*>(&tr);
        return *this;
    }

    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(T const& tr)
    {
        m_cb = &doConstFunctor<T>;
        m_ptr = const_cast<void*>(static_cast<const void*>(&tr));
        return *this;
    }

    // Do not allow temporaries to be stored.
    template <class T>
    constexpr delegate& set(T&&) = delete;

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    static constexpr delegate make()
    {
        return delegate{fkn ? &doFreeCB<fkn> : &doNullFkn, nullptr};
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    static constexpr delegate make(T& o)
    {
        return delegate{&doMemberCB<T, memFkn>, static_cast<void*>(&o)};
    }

    template <class T, R (T::*memFkn)(Args... args) const>
    static constexpr delegate make(const T& o)
    {
        return delegate{&doConstMemberCB<T, memFkn>,
                        const_cast<void*>(static_cast<const void*>(&o))};
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    static constexpr delegate make(T& o)
    {
        return delegate{&doFunctor<T>, static_cast<void*>(&o)};
    }
    template <class T>
    static constexpr delegate make(T const& o)
    {
        return delegate{&doConstFunctor<T>,
                        const_cast<void*>(static_cast<const void*>(&o))};
    }
    template <class T>
    static constexpr delegate make(T&& object) = delete;

    /**
     * Create a delegate to a free function, where the first argument is
     * assumed to be a reference to the object supplied as argument here.
     * The return value and rest of the argument must match the signature
     * of the delegate.
     */
    template <typename T, R (*fkn)(T&, Args...)>
    static constexpr delegate make(T& o)
    {
        return delegate{&dofreeFknWithObjectRef<T, fkn>,
                        static_cast<void*>(&o)};
    }

    template <typename T, R (*fkn)(T const&, Args...)>
    static constexpr delegate make(T& o)
    {
        return delegate{&dofreeFknWithObjectConstRef<T, fkn>,
                        const_cast<void*>(static_cast<void const*>(&o))};
    }

    template <typename T, R (*fkn)(T&, Args...)>
    static constexpr delegate make(T&&) = delete;
    template <typename T, R (*fkn)(T const&, Args...)>
    static constexpr delegate make(T&&) = delete;

    /**
     * Create a callback to a free function with a signature R(void*, Args...)
     * When calling, add the stored void* pointer as first argument.
     */
    template <Trampoline fkn>
    static constexpr delegate makeVoidCB(void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

    /**
     * Create a callback to a free function with a signature R(void*, Args...)
     * When calling, add the stored void* pointer as first argument.
     * Accept pointer as runtime argument.
     */
    static constexpr delegate makeVoidCB(Trampoline fkn, void* ptr = nullptr)
    {
        return delegate(fkn, ptr);
    }

    /**
     * Create a MemFkn object for storing member function before connecting
     * them with an object to make them callable.
     * Keeps track of constness of the member function and have richer type
     * information compared to the delegate.
     */
    template <class T, R (T::*memFkn_)(Args... args) const>
    static constexpr MemFkn<delegate, true> memFkn()
    {
        return MemFkn<delegate, true>{&doConstMemberCB<T, memFkn_>};
    }

    template <class T, R (T::*memFkn_)(Args... args)>
    static constexpr MemFkn<delegate, false> memFkn()
    {
        return MemFkn<delegate, false>{&doMemberCB<T, memFkn_>};
    }

    /**
     * Combine a MemFkn with an object to set this delegate.
     */
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(MemFkn<delegate, false> f, T& o)
    {
        m_cb = f.trampoline;
        m_ptr = static_cast<void*>(&o);
        return *this;
    }
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(MemFkn<delegate, true> f, T const& o)
    {
        m_cb = f.trampoline;
        m_ptr = const_cast<void*>(static_cast<const void*>(&o));
        return *this;
    }
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(MemFkn<delegate, true> f, T& o)
    {
        return set(f, static_cast<T const&>(o));
    }

    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(MemFkn<delegate, true> f,
                                          T&& o) = delete;
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(MemFkn<delegate, false> f,
                                          T&& o) = delete;

    template <class T>
    static constexpr delegate make(MemFkn<delegate, false> f, T& o)
    {
        return delegate{f.trampoline, static_cast<void*>(&o)};
    }
    template <class T>
    static constexpr delegate make(MemFkn<delegate, false>, T const&) = delete;

    template <class T>
    static constexpr delegate make(MemFkn<delegate, true> f, T const& o)
    {
        return delegate{f.trampoline,
                        const_cast<void*>(static_cast<const void*>(&o))};
    }
    template <class T>
    static constexpr delegate make(MemFkn<delegate, true> f, T& o)
    {
        return delegate{f.trampoline, static_cast<void*>(&o)};
    }

    template <class T>
    static constexpr delegate make(MemFkn<delegate, false>, T&&) = delete;
    template <class T>
    static constexpr delegate make(MemFkn<delegate, true>, T&&) = delete;

    // Helper struct to deduce member function types and const.
    template <typename T, T>
    struct DeduceMemberType;

    template <typename T, R (T::*mf)(Args...)>
    struct DeduceMemberType<R (T::*)(Args...), mf>
    {
        using ObjType = T;
        static constexpr bool cnst = false;
        static constexpr Trampoline trampoline = &doMemberCB<ObjType, mf>;
        static constexpr void* castPtr(ObjType* obj)
        {
            return static_cast<void*>(obj);
        }
    };
    template <typename T, R (T::*mf)(Args...) const>
    struct DeduceMemberType<R (T::*)(Args...) const, mf>
    {
        using ObjType = T;
        static constexpr bool cnst = true;
        static constexpr Trampoline trampoline = &doConstMemberCB<ObjType, mf>;
        static constexpr void* castPtr(ObjType const* obj)
        {
            return const_cast<void*>(static_cast<const void*>(obj));
        };
    };

    // C++17 allow template<auto> for non type template arguments.
    // Use to avoid specifying object type.
#if __cplusplus >= 201703

    template <auto mFkn>
    constexpr delegate&
    set(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType& obj)
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        m_cb = DM::trampoline;
        m_ptr = DM::castPtr(&obj);
        return *this;
    }
    template <auto mFkn>
    constexpr delegate&
    set(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType const& obj)
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        m_cb = DM::trampoline;
        m_ptr = DM::castPtr(&obj);
        return *this;
    }
    template <auto mFkn>
    constexpr delegate&
    set(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType&& obj) =
        delete;

    template <auto mFkn>
    static constexpr delegate
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType& obj)
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        return delegate{DM::trampoline, DM::castPtr(&obj)};
    }
    template <auto mFkn>
    static constexpr delegate
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType const& obj)
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        return delegate{DM::trampoline, DM::castPtr(&obj)};
    }

    // Temoraries not allowed.
    template <auto mFkn>
    static constexpr delegate
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType&&) = delete;

    // MemFkn construction.
    template <auto mFkn>
    static constexpr auto memFkn()
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        return MemFkn<delegate, DM::cnst>{DM::trampoline};
    }
#endif

    // Create ordinary free function pointer callback.
    constexpr delegate(Trampoline cb, void* ptr) : m_cb(cb), m_ptr(ptr) {}

  private:
    Trampoline m_cb;
    void* m_ptr;
};

template <typename R, typename... Args>
constexpr bool
operator==(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs)
{
    return lhs.equal(rhs);
}

template <typename R, typename... Args>
constexpr bool
operator!=(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs)
{
    return !(lhs == rhs);
}

// Bite the bullet, this is how unique_ptr handle nullptr_t.
template <typename R, typename... Args>
constexpr bool
operator==(std::nullptr_t lhs, const delegate<R(Args...)>& rhs)
{
    return rhs.null();
}

template <typename R, typename... Args>
constexpr bool
operator!=(std::nullptr_t lhs, const delegate<R(Args...)>& rhs)
{
    return !(lhs == rhs);
}

template <typename R, typename... Args>
constexpr bool
operator==(const delegate<R(Args...)>& lhs, std::nullptr_t rhs)
{
    return lhs.null();
}

template <typename R, typename... Args>
constexpr bool
operator!=(const delegate<R(Args...)>& lhs, std::nullptr_t rhs)
{
    return !(lhs == rhs);
}

// No ordering operators ( operator< etc) defined. This delegate
// represent several classes of pointers and is not a naturally
// ordered type.

/**
 * Helper macro to create a delegate for calling a member function.
 * Example of use:
 *
 * auto cb = DELEGATE_MKMEM(void(), &SomeClass::memberFunction, obj);
 *
 * where 'obj' is of type 'SomeClass'.
 *
 * @param signature Template parameter for the delegate.
 * @param memFknPtr address of member function pointer. C++ require
 *                  full name path with addressof operator (&)
 * @object object which the member function should be called on.
 */
#define DELEGATE_MKMEM(signature, memFknPtr, object)                      \
    (delegate<signature>::make<std::remove_reference_t<decltype(object)>, \
                               memFkn>(object))

#undef DELEGATE_14CONSTEXPR

#endif /* UTILITY_CALLBACK_H_ */
