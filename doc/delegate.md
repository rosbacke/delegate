## delegate

Defined in header "delegate/delegate.h"

The class template `delegate` intend to be a reference to callable entities such as free functions, member functions, and functors. It wraps the actual call into a wrapper function. It stores a pointer to it and a reference to the supplied object
to call on. 
The delegate class requires enough type information to define how a call is made, that is,
the return type and arguments of the call. Other type information such as the stored function name and type of function (free, member, functor) is invisible to the caller.

The main intended use case is as an std::function analog where were we guarantee small
memory size (2 pointers), no exceptions, no heap allocation and being trivially_copyable.
A delegate does not store the object to call on, only a reference, so the user must 
ensure the lifetime.

The delegate intends to be similar to a normal function pointer. It should work as a normal pointer (default constructible, compared to nullptr, act as a value type w.r.t. to the stored address and pointer const behavior.)

There are 2 main overload sets to set up delegates, `set` and `make`. 
 * `set`: A templated member function which will set the value for the current object.
 * `make`: A templated static member function which returns a newly constructed delegate.

The actual function to call is supplied via the template argument which allows for creating
the stored wrapper function. Giving the function statically will also allow the compiler to optimize better.
The delegate allows for storing normal runtime function pointers. It makes it possible to store calls to stateless lambdas via lambda to function pointer conversion. Do note we still get the extra indirect call via an internal wrapper. A runtime function can be made via
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
| `set_free_with_object` | See separate section.
| `set_free_with_void` | See separate section.

#### Overload set for *set* member function family

All *set* functions are declared *noexcept*. For *C++14* and newer, it is *constexpr*. They return a reference to its own object.

**Set a free function.**  
Create a wrapper for *fkn* and store in this object.  
`template <R (*fkn)(Args...)>`  
`delegate& set()`

**Set a member function with object reference to call on.**  
Create a wrapper for *mf* member function and store a reference to *obj* to call on.  
`template <class T, R (T::*mf)(Args...)>`  
`delegate& set(T& obj)`

`template <class T, R (T::*mf)(Args...) const>`  
`delegate& set(T const& obj)`

**C++17** Create a wrapper for *mf* member function and store a reference to *obj* to call on. Deduce *T* from *mf*. Only for C++17 and beyond.  
`template <auto mf>`  
`delegate& set(T& obj)`

`template <auto mf>`  
`delegate& set(T const& obj)`  // Require mf to be const declared.

**Set a reference to a functor.**  
Create a wrapper for a functor and store a reference to *obj* to call on.  
`template <class T>`  
`delegate& set(T& obj)`  // Will call non-const operator().

`template <class T>`  
`delegate& set(T const& obj)` // Will call const operator().

**Set a mem_fkn with an object to call on.**  
Store the wrapper from `mem_fkn` and a reference to *obj*.  
`template <class T>`  
`delegate& set(mem_fkn<T, false, R(Args...)> const& mf, T& obj)`

`template <class T>`  
`delegate& set(mem_fkn<T, true, R(Args...)> const& mf, T const& obj)`

`template <class T>`  
`delegate& set(mem_fkn<T, true, R(Args...)> const& mf, T& obj)`

**Store a runtime variable free function pointer.**  
`delegate& set(R (*fkn)(Args...))`

`delegate& set_fkn(R (*fkn)(Args...))`  // Separate function name allow implicit conversion.

**Set a free function with extra void context arg**.  
Call a free function with an extra _void*_ as the first argument. Intended to allow calling back to
legacy _void*_ context pointer-based code.  
`template <typename T, R (*fkn)(void*, Args...)>`  
`delegate& set_free_with_void(void*)`

`template <typename T, R (*fkn)(void const*, Args...)>`  
`delegate& set_free_with_void(void const*)`

**Set a free function with extra object arg**.  
Call a free function with an extra object reference as the first argument. Will pass on `obj` when called.
Intended to allow calling back to code based on free functions, typically found in older systems.  
`template <typename T, R (*fkn)(T&, Args...)>`  
`delegate& set_free_with_object(T& obj)`

`template <typename T, R (*fkn)(T const&, Args...)>`  
`delegate& set_free_with_object(T& obj)`

`template <typename T, R (*fkn)(T const&, Args...)>`  
`delegate& set_free_with_object(T const& obj)`

#### Deleted functions, *set* family.

`template <class T, R (T::*memFkn)(Args... args)>`  
`delegate& set(T&&) =delete;` // No temporaries.  

`template <class T, R (T::*memFkn)(Args... args) const>`  
`delegate& set(T&&) =delete;` // No temporaries.  

`template <class T>`   // No temporaries.  
`delegate& set(T&&) =delete;`

`template <class T>`  // violates const correctness.  
`delegate& set(mem_fkn<T, false, R(Args...)> const& f, T const& o) =delete;`

`template <class T>`   // No temporaries.  
`delegate& set(mem_fkn<T, false, R(Args...)> const& f, T&& o) =delete;`

`template <class T>`   // No temporaries.  
`delegate& set(mem_fkn<T, true, R(Args...)> const& f, T&& o) =delete;`

`template <auto mf>`   // No temporaries.  
`delegate& set(<deduced type T from mf>&& obj) =delete; // >= C++17`

`template <typename T, R (*fkn)(void*, Args...)>` // violates const correctness.  
`delegate& set_free_with_void(void const*) = delete;`

`template <typename T, R (*fkn)(T&, Args...)>` // violates const correctness.  
`delegate& set_free_with_object(T const&) = delete;`

`template <typename T, R (*fkn)(T&, Args...)>` // No temporaries.   
`delegate& set_free_with_object(T&&) = delete;`

`template <typename T, R (*fkn)(T&, Args...) const>` // No temporaries.  
`delegate& set_free_with_object(T&&) = delete;`  

### Static member functions

**equal**. Return _true_ if both arguments point to the same wrapper function and the context pointer is equal, or both delegates are in the null state.  
`bool equal(delegate const&, delegate const&)`

**less**. Return _true_ if the _lhs_ wrapper function is < the _rhs_ wrapper function. If wrapper functions are equal, return true if < is true on context pointers. The null state is < all stored wrappers.  
`bool less(delegate const&, delegate const&)`


#### Overload set for static member function *make*

The entire *make* family of functions are a mirror image of the *set* family.
See the *set* family documentation and do the following transform:
- Make it static member instead of normal member.
- Have it return `delegate` instead of `delegate&`.
- The *make* family is constexpr even for C++11.

### Free functions
#### operator overloads, relations

The basic equality operators `==` and `!=` are defined. They are in terms of static member function `equal`.
They are defined with this pattern:

`template <typename R, typename... Args>`  
`constexpr bool`  
`operator op(const delegate<R(Args...)>& lhs, const delegate<R(Args...)>& rhs) noexcept`
    

#### operator overloads, nullptr

Defined in terms of static member function `null` and `equal`. 

`template <typename R, typename... Args>`  
`constexpr bool `  
`operator==(delegate<R(Args...)> const& lhs, nullptr_t const& rhs) noexcept;`

`template <typename R, typename... Args>`  
`constexpr bool `  
`operator==(nullptr_t const&, delegate<R(Args...)> const& rhs) noexcept;`

`template <typename R, typename... Args>`  
`constexpr bool`  
`operator!=(delegate<R(Args...)> const& lhs, nullptr_t const& rhs) noexcept;`

`template <typename R, typename... Args>`  
`constexpr bool `  
`operator!=(nullptr_t const&, delegate<R(Args...)> const& rhs) noexcept;`
