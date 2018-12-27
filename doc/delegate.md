## delegate

Defined in header "delegate/delegate.h"

The class template `delegate` intend to be a reference to callable entities such as 
free functions, member functions and functors. It wraps the actual call into 
a wrapper function. It stores a pointer to it and a reference to the supplied object
to call on. 
The delegate class require enough type information to define how a call is made, that is,
the return type and arguments of the call. Other type information such as the stored 
function name and type of function (free, member, functor) is invisible to the caller.

The main intended use case is as a std::function analog where were we guarantee small
memory size (2 pointers), no exceptions, no heap allocation and being trivially_copyable.
A delegate does not store the object to call on, only a reference, so the user must 
ensure the lifetime.

The delegate intend to be similar to a normal function pointer. It should work as a normal pointer (default constructible, compared to nullptr, act as a value type w.r.t. to the stored address and pointer const behaviour.)

There are 2 main overload sets to set up delegates, `set` and `make`. 
 * `set` : A templated member function which will set the value for the current object.
 * `make` : A templated static member function which return a newly constructed delegate.

The actual function to call is supplied via the template argument which allow for creating
the stored wrapper function. Giving the function statically will also allow the compiler 
to optimize better.
The delegate allow for storing normal runtime function pointers. It makes it possible to 
store calls to stateless lambdas via lambda to function pointer conversion. Do note we still get the extra indirect call via an internal wrapper. A runtime function can be made via
a delegate constructor call.

---

``` cpp
    template <typename Signature>
    class delegate;
    template<class R, class... Args>
    class delegate<R(Args...)> 
```

| Type parameters | Description
| --- | ---
| **Signature** | Signature for function call. Contain return type and argument types, similar to std::function.
| **R** | Return type from the called function.
| **Args** | Parameter pack with parameters for the function call.

### Member types

| Member types |  Description
| --- | ---
| `Equal` | Binary predicate functor. Forward call to static member function *equal*. Intended to work as a standard STL binary predicate.
| `Less` | Binary predicate functor. Forward call to static member function *less*. Intended to work as a standard STL binary predicate.

### Member functions

| Member function |  Description
| --- | ---
| `delegate()` | Default construct a *delegate*. Post-condition: `this->null() == true.`
| `delegate(R (*fkn)(Args...))` | Construct a *delegate*. Will call *fkn* on call. 
| `bool null() const` | Return _true_ if the mem_fkn is in a null state.
| `void clear()`| Set *delegate* to null state. Post-condition: `this->null() == true.`
| `explicit operator bool() const`| Return `!null()`
| `R operator()(Args...) const`| Call the stored wrapper fkn. Valid to call when *null()==true*
| `set` | See separate overload set table.
| `delegate& set_fkn(R (*fkn)(Args...)` | Same as *set*. With unique member name it can implicitly convert from stateless lambda to a free function.


##### Overload set for member function *set*

All *set* functions are declared *noexcept*. For *C++14* and newer, it is *constexpr*. They return a reference to its own object.

| *set* member function |  Description
| --- | ---
| `template <R (*fkn)(Args...)> delegate& set()` | Set delegate to call *fkn*.
| `template <class T, R (T::*mf)(Args...)> delegate& set(T& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. 
| `template <class T, R (T::*mf)(Args...) const> delegate& set(T const& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. 
| `template <class T> delegate& set(T& obj)` | Create a wrapper for a functor and store a reference to *obj* to call on. Will call non-const `operator()`
| `template <class T> delegate& set(T const& obj)` | Create a wrapper for a functor and store a reference to *obj* to call on. Will call const `operator()`
| `template <class T> delegate& set(mem_fkn<T, false, R(Args...)> const& mf, T& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `template <class T> delegate& set(mem_fkn<T, true, R(Args...)> const& mf, T const& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `template <class T> delegate& set(mem_fkn<T, true, R(Args...)> const& mf, T& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `delegate& set(R (*fkn)(Args...))` | Store a runtime variable free function pointer.

| *set*, C++17 and newer | Description
| --- | ---
| `template <auto mf> delegate& set(T& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. Deduce *T* from *mf*. Deduce to non const *mf*.
| `template <auto mf> delegate& set(T const& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. Deduce *T* from *mf*. Require const *mf*.

| *set*, Deleted functions.
| ---
| `template <class T, R (T::*memFkn)(Args... args)> delegate& set(T&&) =delete;`
| `template <class T, R (T::*memFkn)(Args... args) const> delegate& set(T&&) =delete;`
| `template <class T> delegate& set(T&&) =delete;`
| `template <class T> delegate& set(mem_fkn<T, false, R(Args...)> const& f, T const& o) =delete;`
| `template <class T> delegate& set(mem_fkn<T, false, R(Args...)> const& f, T&& o) =delete;`	
| `template <class T> delegate& set(mem_fkn<T, true, R(Args...)> const& f, T&& o) =delete;`
| `template <auto mf> delegate& set(<deduced type T from mf>&& obj) =delete; // >= C++17`

### Static member functions

| Static function |  Description
| --- | ---
| `bool equal(mem_fkn const&, mem_fkn const&)` | Return _true_ if both arguments point to the same wrapper function or both are in null state.
| `bool less(mem_fkn const&, mem_fkn const&)` | Return _true_ if the _lhs_ wrapper function is < the _rhs_ wrapper function. The null state is < all stored wrappers.
| `make` | See separate overload set table.
| `delegate make_fkn(R (*fkn)(Args...)` | Same as *make*. With unique member name it can implicitly convert from stateless lambda to a free function.

##### Overload set for static member function *make*

All *make* functions are declared *noexcept* and *constexpr*. They return a newly constructed *delegate*.

| *make* static member function |  Description
| --- | ---
| `template <R (*fkn)(Args...)> delegate make()` | Returned delegate will call *fkn*.
| `template <class T, R (T::*mf)(Args...)> delegate make(T& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. 
| `template <class T, R (T::*mf)(Args...) const> delegate make(T const& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. 
| `template <class T> delegate make(T& obj)` | Create a wrapper for a functor and store a reference to *obj* to call on. Will call non-const `operator()`
| `template <class T> delegate make(T const& obj)` | Create a wrapper for a functor and store a reference to *obj* to call on. Will call const `operator()`
| `template <class T> delegate make(mem_fkn<T, false, R(Args...)> const& mf, T& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `template <class T> delegate make(mem_fkn<T, true, R(Args...)> const& mf, T const& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `template <class T> delegate make(mem_fkn<T, true, R(Args...)> const& mf, T& obj)` | Store the wrapper from `mem_fkn` and a reference to *obj*.
| `delegate& make(R (*fkn)(Args...))` | Store a runtime variable free function pointer.

| *make*, C++17 and newer | Description
| --- | ---
| `template <auto mf> delegate make(T& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. Deduce *T* from *mf*. Deduce to non const *mf*.
| `template <auto mf> delegate make(T const& obj)` | Create a wrapper for *mf* member function and store a reference to *obj* to call on. Deduce *T* from *mf*. Require const *mf*.

| *make*, Deleted functions.
| ---
| `template <class T, R (T::*memFkn)(Args... args)> delegate make(T&&) =delete;`
| `template <class T, R (T::*memFkn)(Args... args) const> delegate make(T&&) =delete;`
| `template <class T> delegate make(T&&) =delete;`
| `template <class T> delegate make(mem_fkn<T, false, R(Args...)> const& f, T const& o) =delete;`
| `template <class T> delegate make(mem_fkn<T, false, R(Args...)> const& f, T&& o) =delete;`	
| `template <class T> delegate make(mem_fkn<T, true, R(Args...)> const& f, T&& o) =delete;`
| `template <auto mf> delegate make(<deduced type T from mf>&& obj) =delete; // >= C++17`

### Free functions, 
#### operator overloads, relations

The basic relational operators. Defined in terms of static member functions `null`, `equal` and `less`. 
They are defined with this pattern:

    template <typename R, typename... Args> constexpr bool operator op(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs) noexcept
    
    The following operators are defined: 
        ==, !=, <, <=, =>, > with this pattern.

#### operator overloads, nullptr

Defined in terms of static member functions `null`, `equal` and `less`. 

| operator overload nullptr
| ---
| template <typename R, typename... Args> constexpr bool operator==(delegate<R(Args...)> const& lhs, nullptr_t const& rhs) noexcept;
| template <typename R, typename... Args> constexpr bool operator==(nullptr_t const&, delegate<R(Args...)> const& rhs) noexcept;
| template <typename R, typename... Args> constexpr bool operator!=(delegate<R(Args...)> const& lhs, nullptr_t const& rhs) noexcept;
| template <typename R, typename... Args> constexpr bool operator!=(nullptr_t const&, delegate<R(Args...)> const& rhs) noexcept;

