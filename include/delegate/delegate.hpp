/*
 * delegate.h
 *
 *  Created on: 8 aug. 2017
 *      Author: Mikael Rosbacke
 */

#ifndef DELEGATE_DELEGATE_HPP_
#define DELEGATE_DELEGATE_HPP_

#include <cstddef> // nullptr_t
#include <cstdint> // uintptr_t

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

template <typename T>
class common;

template <typename R, typename... Args>
class common<R(Args...)>
{
  public:
    union DataPtr {
        using FknPtr = R (*)(Args...);

        constexpr DataPtr() = default;
        constexpr DataPtr(void* p) noexcept : v_ptr(p){};
        constexpr DataPtr(FknPtr p) noexcept : fkn_ptr(p){};

        void* v_ptr = nullptr;
        FknPtr fkn_ptr;
    };

    using Trampoline = R (*)(DataPtr const&, Args...);

    inline static constexpr R doNullCB(DataPtr const& o, Args... args)
    {
        return nullReturnFunction<R>();
    }

    // Adapter function for the member + object calling.
    template <class T, R (T::*memFkn)(Args...)>
    inline static constexpr R doMemberCB(DataPtr const& o, Args... args)
    {
        return (static_cast<T*>(o.v_ptr)->*memFkn)(args...);
    }

    // Adapter function for the member + object calling.
    template <class T, R (T::*memFkn)(Args...) const>
    inline static constexpr R doConstMemberCB(DataPtr const& o, Args... args)
    {
        return (static_cast<T const*>(o.v_ptr)->*memFkn)(args...);
    }
};

} // namespace details

template <typename T>
class delegate;

/**
 * Simple adapter class for holding member functions and
 * allow them to be called similar to std::invoke.
 *
 * @param T Object type this member can be called on.
 * @param cnst Force stored member function to be const.
 * @param Signature The signature required to call this member function.
 *
 * Notes on the cnst arguments. If false, we require the member to be called
 * only on non const objects. Normally we match only on non const member
 * functions. When true we require member functions to be const, so they can be
 * called on both non-const/const objects. To assign a const function to a
 * mem_fkn which is false, use make_from_const. A separate function name is
 * needed to avoid ambiguous overloads w.r.t. constness.
 *
 * Current idea: Give both T and constness separately to be part of the type.
 * - Allow us to disambiguate const/non-const member functions when taking the
 * address.
 *
 * Discarded idea: T can be const/non-const. If const is  not part of stored
 * type, we can not later use constness to disambiguate taking the address.
 * Still, constness of function is different from constness of object.
 *
 * ->
 */
template <class T, bool, typename Signature>
class mem_fkn;

template <typename Signature>
class mem_fkn_base;

template <typename R_, typename... Args>
class mem_fkn_base<R_(Args...)>
{
  protected:
    using R = R_;
    using common = details::common<R(Args...)>;
    using DataPtr = typename common::DataPtr;
    using Trampoline = typename common::Trampoline;

    mem_fkn_base(Trampoline fkn) : fknPtr(fkn){};
    mem_fkn_base() = default;

    Trampoline fknPtr = common::doNullCB;

    void setPtr(Trampoline t)
    {
        fknPtr = t;
    }

  public:
    Trampoline ptr() const
    {
        return fknPtr;
    }

    constexpr bool null() const noexcept
    {
        return fknPtr == common::doNullCB;
    }
    // Return true if a function pointer is stored.
    constexpr explicit operator bool() const noexcept
    {
        return !null();
    }
    static constexpr bool equal(const mem_fkn_base& lhs,
                                const mem_fkn_base& rhs)
    {
        return lhs.fknPtr == rhs.fknPtr;
    }
    static constexpr bool less(const mem_fkn_base& lhs, const mem_fkn_base& rhs)
    {
        return (lhs.null() && !rhs.null()) || (lhs.fknPtr < rhs.fknPtr);
    }
};

template <typename T, typename R_, typename... Args>
class mem_fkn<T, false, R_(Args...)> : public mem_fkn_base<R_(Args...)>
{
    using R = R_;
    using Base = class mem_fkn_base<R_(Args...)>;
    static constexpr const bool cnst = false;
    using common = details::common<R(Args...)>;
    using DataPtr = typename common::DataPtr;
    using Trampoline = typename common::Trampoline;

    mem_fkn(Trampoline fkn) : Base(fkn){};

  public:
    mem_fkn() = default;

    template <typename U>
    R invoke(U&& o, Args... args)
    {
        return Base::fknPtr(DataPtr{static_cast<void*>(&o)}, args...);
    }

    template <R (T::*memFkn_)(Args... args)>
    DELEGATE_CXX14CONSTEXPR mem_fkn& set() noexcept
    {
        Base::setPtr(&common::template doMemberCB<T, memFkn_>);
        return *this;
    }
    template <R (T::*memFkn_)(Args... args) const>
    DELEGATE_CXX14CONSTEXPR mem_fkn& set_from_const() noexcept
    {
        Base::fknPtr = &common::template doConstMemberCB<T, memFkn_>;
        return *this;
    }
    template <R (T::*memFkn_)(Args... args)>
    static constexpr mem_fkn make() noexcept
    {
        return mem_fkn{&common::template doMemberCB<T, memFkn_>};
    }
    template <R (T::*memFkn_)(Args... args) const>
    static constexpr mem_fkn make_from_const() noexcept
    {
        return mem_fkn{&common::template doConstMemberCB<T, memFkn_>};
    }
};

template <typename T, typename R_, typename... Args>
class mem_fkn<T, true, R_(Args...)> : public mem_fkn_base<R_(Args...)>
{
    using R = R_;
    using Base = mem_fkn_base<R_(Args...)>;
    static constexpr const bool cnst = true;
    using common = details::common<R(Args...)>;
    using Trampoline = typename common::Trampoline;

    mem_fkn(Trampoline fkn) : Base(fkn){};

  public:
    mem_fkn() = default;

    R invoke(T const& o, Args... args)
    {
        return Base::fknPtr(const_cast<T*>(&o), args...);
    }

    template <R (T::*memFkn_)(Args... args) const>
    static constexpr mem_fkn make() noexcept
    {
        return mem_fkn{&common::template doConstMemberCB<T, memFkn_>};
    }
    template <R (T::*memFkn_)(Args... args) const>
    DELEGATE_CXX14CONSTEXPR mem_fkn& set() noexcept
    {
        Base::fknPtr = &common::template doConstMemberCB<T, memFkn_>;
        return *this;
    }
};

template <typename T, bool cnst, typename S>
bool
operator==(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return lhs.equal(lhs, rhs);
}

template <typename T, bool cnst, typename S>
bool
operator!=(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, bool cnst, typename S>
bool
operator<(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return lhs.less(lhs, rhs);
}

template <typename T, bool cnst, typename S>
bool
operator<=(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return lhs.less(lhs, rhs) || lhs.equal(lhs, rhs);
}

template <typename T, bool cnst, typename S>
bool
operator>=(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return lhs.less(rhs, lhs) || lhs.equal(lhs, rhs);
}

template <typename T, bool cnst, typename S>
bool
operator>(const mem_fkn<T, cnst, S>& lhs, const mem_fkn<T, cnst, S>& rhs)
{
    return rhs.less(rhs, lhs);
}

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
    using common = details::common<R(Args...)>;
    using DataPtr = typename common::DataPtr;

    // Signature presented to the user when calling the callback.
    using TargetFreeCB = R (*)(Args...);

    // Type of the function pointer for the trampoline functions.
    using Trampoline = typename common::Trampoline;

    // Adaptor function for the case where void* is not forwarded
    // to the caller. (Just a normal function pointer.)
    template <R(freeFkn)(Args...)>
    inline static R doFreeCB(DataPtr const& v, Args... args)
    {
        return freeFkn(args...);
    }

    // Adapter function for when the stored object is a pointer to a
    // callable object (stored elsewhere). Call it using operator().
    template <class Functor>
    inline static R doFunctor(DataPtr const& o_arg, Args... args)
    {
        auto obj = static_cast<Functor*>(o_arg.v_ptr);
        return (*obj)(args...);
    }

    template <class Functor>
    inline static R doConstFunctor(DataPtr const& o_arg, Args... args)
    {
        const Functor* obj = static_cast<Functor const*>(o_arg.v_ptr);
        return (*obj)(args...);
    }

    inline static R doRuntimeFkn(DataPtr const& o_arg, Args... args)
    {
        TargetFreeCB fkn = o_arg.fkn_ptr;
        return fkn(args...);
    }

    // Adapter function for the free function with extra first arg
    // in the called function, set at delegate construction.
    template <class T, R(freeFkn)(T&, Args...)>
    inline static R dofreeFknWithObjectRef(DataPtr const& o, Args... args)
    {
        T* obj = static_cast<T*>(o.v_ptr);
        return freeFkn(*obj, args...);
    }

    // Adapter function for the free function with extra first arg
    // in the called function, set at delegate construction.
    template <class T, R(freeFkn)(T const&, Args...)>
    inline static R dofreeFknWithObjectConstRef(DataPtr const& o, Args... args)
    {
        T const* obj = static_cast<const T*>(o.v_ptr);
        return freeFkn(*obj, args...);
    }

  public:
    // Default construct with stored ptr == nullptr.
    constexpr delegate() = default;

    // General simple function pointer handling. Will accept stateless lambdas.
    // Do note it is less easy to optimize compared to static function setup.
    // Handle nullptr / 0 arguments as well.
    constexpr delegate(TargetFreeCB fkn) noexcept
        : m_cb(fkn == nullptr ? &common::doNullCB : &doRuntimeFkn),
          m_ptr(fkn){};

    ~delegate() = default;

    // Call the stored function. Requires: bool(*this) == true;
    // Will call trampoline fkn which will call the final fkn.
    constexpr R operator()(Args... args) const __attribute__((always_inline))
    {
        return m_cb(m_ptr, args...);
    }

    constexpr bool null() const noexcept
    {
        return m_cb == &common::doNullCB;
    }

    static constexpr bool equal(const delegate& lhs,
                                const delegate& rhs) noexcept
    {
        // Ugly, but the m_ptr part should be optimized away on normal
        // platforms.
        return lhs.m_cb == rhs.m_cb &&
               (lhs.m_cb == doRuntimeFkn
                    ? (lhs.m_ptr.fkn_ptr == rhs.m_ptr.fkn_ptr)
                    : (lhs.m_ptr.v_ptr == rhs.m_ptr.v_ptr));
    }

    constexpr bool equal(const delegate& rhs) const noexcept
    {
        return equal(*this, rhs);
    }

    // Helper Functor for passing into std functions etc.
    struct Equal
    {
        constexpr bool operator()(const delegate& lhs,
                                  const delegate& rhs) const noexcept
        {
            return equal(lhs, rhs);
        }
    };

    // Define a total order for purpose of sorting in maps etc.
    // Do not define operators since this is not a natural total order.
    // It will vary randomly depending on where symbols end up etc.
    static constexpr bool less(const delegate& lhs,
                               const delegate& rhs) noexcept
    {
        // Ugly, but the m_ptr part should be optimized away on normal
        // platforms.
        return (lhs.null() && !rhs.null()) //
               || lhs.m_cb < rhs.m_cb      //
               || (lhs.m_cb == rhs.m_cb &&
                   (lhs.m_cb == doRuntimeFkn
                        ? (lhs.m_ptr.fkn_ptr < rhs.m_ptr.fkn_ptr)
                        : (lhs.m_ptr.v_ptr < rhs.m_ptr.v_ptr)));
    }

    constexpr bool less(const delegate& rhs) const noexcept
    {
        return less(*this, rhs);
    }

    // Helper Functor for passing into std::map et.al.
    struct Less
    {
        constexpr bool operator()(const delegate& lhs,
                                  const delegate& rhs) const noexcept
        {
            return less(lhs, rhs);
        }
    };

    // Return true if a function pointer is stored.
    constexpr explicit operator bool() const noexcept
    {
        return !null();
    }

    DELEGATE_CXX14CONSTEXPR void clear() noexcept
    {
        m_cb = common::doNullCB;
        m_ptr.v_ptr = nullptr;
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    DELEGATE_CXX14CONSTEXPR delegate& set() noexcept
    {
        m_cb = &doFreeCB<fkn>;
        m_ptr.v_ptr = nullptr;
        return *this;
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    DELEGATE_CXX14CONSTEXPR delegate& set(T& tr) noexcept
    {
        m_cb = &common::template doMemberCB<T, memFkn>;
        m_ptr.v_ptr = static_cast<void*>(&tr);
        return *this;
    }

    template <class T, R (T::*memFkn)(Args... args) const>
    DELEGATE_CXX14CONSTEXPR delegate& set(T const& tr) noexcept
    {
        m_cb = &common::template doConstMemberCB<T, memFkn>;
        m_ptr.v_ptr = const_cast<void*>(static_cast<const void*>(&tr));
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
    DELEGATE_CXX14CONSTEXPR delegate& set(T& tr) noexcept
    {
        m_cb = &doFunctor<T>;
        m_ptr.v_ptr = static_cast<void*>(&tr);
        return *this;
    }

    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(T const& tr) noexcept
    {
        m_cb = &doConstFunctor<T>;
        m_ptr.v_ptr = const_cast<void*>(static_cast<const void*>(&tr));
        return *this;
    }

    // Do not allow temporaries to be stored.
    template <class T>
    constexpr delegate& set(T&&) const = delete;

    DELEGATE_CXX14CONSTEXPR delegate& set(TargetFreeCB fkn) noexcept
    {
        m_cb = &doRuntimeFkn;
        m_ptr.fkn_ptr = fkn;
        return *this;
    }

    DELEGATE_CXX14CONSTEXPR delegate& set_fkn(TargetFreeCB fkn) noexcept
    {
        return set(fkn);
    }

    /**
     * Create a callback to a free function with a specific type on
     * the pointer.
     */
    template <R (*fkn)(Args... args)>
    static constexpr delegate make() noexcept
    {
        // Note: template arg can never be nullptr.
        return delegate{&doFreeCB<fkn>, static_cast<void*>(nullptr)};
    }

    /**
     * Create a callback to a member function to a given object.
     */
    template <class T, R (T::*memFkn)(Args... args)>
    static constexpr delegate make(T& o) noexcept
    {
        return delegate{&common::template doMemberCB<T, memFkn>,
                        static_cast<void*>(&o)};
    }

    template <class T, R (T::*memFkn)(Args... args) const>
    static constexpr delegate make(const T& o) noexcept
    {
        return delegate{&common::template doConstMemberCB<T, memFkn>,
                        const_cast<void*>(static_cast<const void*>(&o))};
    }

    /**
     * Create a callback to a Functor or a lambda.
     * NOTE : Only a pointer to the functor is stored. The
     * user must ensure the functor is still valid at call time.
     * Hence, we do not accept functor r-values.
     */
    template <class T>
    static constexpr delegate make(T& o) noexcept
    {
        return delegate{&doFunctor<T>, static_cast<void*>(&o)};
    }
    template <class T>
    static constexpr delegate make(T const& o) noexcept
    {
        return delegate{&doConstFunctor<T>,
                        const_cast<void*>(static_cast<const void*>(&o))};
    }
    template <class T>
    static constexpr delegate make(T&& object) = delete;

    static constexpr delegate make(TargetFreeCB fkn) noexcept
    {
        return delegate{&doRuntimeFkn, fkn};
    }

    static constexpr delegate make_fkn(TargetFreeCB fkn) noexcept
    {
        return delegate{&doRuntimeFkn, fkn};
    }

    /**
     * Create a delegate to a free function, where the first argument is
     * assumed to be a reference to the object supplied as argument here.
     * The return value and rest of the argument must match the signature
     * of the delegate.
     */
    template <typename T, R (*fkn)(T&, Args...)>
    static constexpr delegate make(T& o) noexcept
    {
        return delegate{&dofreeFknWithObjectRef<T, fkn>,
                        static_cast<void*>(&o)};
    }

    template <typename T, R (*fkn)(T const&, Args...)>
    static constexpr delegate make(T& o) noexcept
    {
        return delegate{&dofreeFknWithObjectConstRef<T, fkn>,
                        static_cast<void const*>(&o)};
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
    static constexpr delegate makeVoidCB(void* ptr = nullptr) noexcept
    {
        return delegate(fkn, ptr);
    }

    /**
     * Create a callback to a free function with a signature R(void*, Args...)
     * When calling, add the stored void* pointer as first argument.
     * Accept pointer as runtime argument.
     */
    static constexpr delegate makeVoidCB(Trampoline fkn,
                                         void* ptr = nullptr) noexcept
    {
        return delegate(fkn, ptr);
    }

    /**
     * Combine a MemFkn with an object to set this delegate.
     */
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(mem_fkn<T, false, R(Args...)> f,
                                          T& o) noexcept
    {
        m_cb = f.ptr();
        m_ptr = static_cast<void*>(&o);
        return *this;
    }
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(mem_fkn<T, false, R(Args...)> f,
                                          T const& o) = delete;

    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(mem_fkn<T, true, R(Args...)> f,
                                          T const& o) noexcept
    {
        m_cb = f.ptr();
        m_ptr = const_cast<void*>(static_cast<const void*>(&o));
        return *this;
    }
    template <class T>
    DELEGATE_CXX14CONSTEXPR delegate& set(mem_fkn<T, true, R(Args...)> f,
                                          T& o) noexcept
    {
        return set(f, static_cast<T const&>(o));
    }

    template <class T, bool cnst>
    DELEGATE_CXX14CONSTEXPR delegate& set(mem_fkn<T, cnst, R(Args...)> f,
                                          T&& o) = delete;

    template <class T>
    static constexpr delegate make(mem_fkn<T, false, R(Args...)> f,
                                   T& o) noexcept
    {
        return delegate{f.ptr(), static_cast<void*>(&o)};
    }
    template <class T>
    static constexpr delegate make(mem_fkn<T, false, R(Args...)>,
                                   T const&) = delete;

    template <class T>
    static constexpr delegate make(mem_fkn<T, true, R(Args...)> f,
                                   T const& o) noexcept
    {
        return delegate{f.ptr(),
                        const_cast<void*>(static_cast<const void*>(&o))};
    }
    template <class T>
    static constexpr delegate make(mem_fkn<T, true, R(Args...)> f,
                                   T& o) noexcept
    {
        return delegate{f.ptr(), static_cast<void*>(&o)};
    }

    template <class T, bool cnst>
    static constexpr delegate make(mem_fkn<T, cnst, R(Args...)>, T&&) = delete;

    // Helper struct to deduce member function types and const.
    template <typename T, T>
    struct DeduceMemberType;

    template <typename T, R (T::*mf)(Args...)>
    struct DeduceMemberType<R (T::*)(Args...), mf>
    {
        using ObjType = T;
        static constexpr bool cnst = false;
        static constexpr Trampoline trampoline =
            &common::template doMemberCB<ObjType, mf>;
        static constexpr void* castPtr(ObjType* obj) noexcept
        {
            return static_cast<void*>(obj);
        }
    };
    template <typename T, R (T::*mf)(Args...) const>
    struct DeduceMemberType<R (T::*)(Args...) const, mf>
    {
        using ObjType = T;
        static constexpr bool cnst = true;
        static constexpr Trampoline trampoline =
            &common::template doConstMemberCB<ObjType, mf>;
        static constexpr void* castPtr(ObjType const* obj) noexcept
        {
            return const_cast<void*>(static_cast<const void*>(obj));
        };
    };

    // C++17 allow template<auto> for non type template arguments.
    // Use to avoid specifying object type.
#if __cplusplus >= 201703

    template <auto mFkn>
    constexpr delegate&
    set(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType& obj) noexcept
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        m_cb = DM::trampoline;
        m_ptr = DM::castPtr(&obj);
        return *this;
    }
    template <auto mFkn>
    constexpr delegate&
    set(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType const&
            obj) noexcept
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
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType& obj) noexcept
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        return delegate{DM::trampoline, DM::castPtr(&obj)};
    }
    template <auto mFkn>
    static constexpr delegate
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType const&
             obj) noexcept
    {
        using DM = DeduceMemberType<decltype(mFkn), mFkn>;
        return delegate{DM::trampoline, DM::castPtr(&obj)};
    }

    // Temporaries not allowed.
    template <auto mFkn>
    static constexpr delegate
    make(typename DeduceMemberType<decltype(mFkn), mFkn>::ObjType&&) = delete;

#endif

  private:
    // Create ordinary free function pointer callback.
    constexpr delegate(Trampoline cb, void* ptr) noexcept : m_cb(cb), m_ptr(ptr)
    {
    }

    // Create ordinary free function pointer callback.
    constexpr delegate(Trampoline cb, const void* ptr) noexcept
        : m_cb(cb), m_ptr(ptr)
    {
    }

    constexpr delegate(Trampoline cb, TargetFreeCB fkn) noexcept
        : m_cb(cb), m_ptr(fkn)
    {
    }

    Trampoline m_cb = &common::doNullCB;
    DataPtr m_ptr;
};

template <typename R, typename... Args>
constexpr bool
operator==(const delegate<R(Args...)>& lhs,
           const delegate<R(Args...)>& rhs) noexcept
{
    return delegate<R(Args...)>::equal(lhs, rhs);
}

template <typename R, typename... Args>
constexpr bool
operator!=(const delegate<R(Args...)>& lhs,
           const delegate<R(Args...)>& rhs) noexcept
{
    return !(lhs == rhs);
}

// Bite the bullet, this is how unique_ptr handle nullptr_t.
template <typename R, typename... Args>
constexpr bool
operator==(std::nullptr_t lhs, const delegate<R(Args...)>& rhs) noexcept
{
    return rhs.null();
}

template <typename R, typename... Args>
constexpr bool
operator!=(std::nullptr_t lhs, const delegate<R(Args...)>& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename R, typename... Args>
constexpr bool
operator==(const delegate<R(Args...)>& lhs, std::nullptr_t rhs) noexcept
{
    return lhs.null();
}

template <typename R, typename... Args>
constexpr bool
operator!=(const delegate<R(Args...)>& lhs, std::nullptr_t rhs) noexcept
{
    return !(lhs == rhs);
}

// No ordering operators ( operator< etc) defined. This delegate
// represent several classes of pointers and is not a naturally
// ordered type. Use members less, Less for explicit ordering.

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
