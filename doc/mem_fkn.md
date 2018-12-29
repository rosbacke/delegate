## mem_fkn

Defined in header "delegate/delegate.h"

Wrap a member function into a wrapper function and stores a pointer to this function.
Keeps enough type information about the member function to later call it in a 
type-safe manner.
The member function is supplied as a template value argument at construction and is used to synthesize a wrapper function. A pointer to the wrapper function is stored in the mem_fkn object.
Once stored the `mem_fkn` only contain enough type information to safely call the stored wrapper.

A `mem_fkn` in null state points to a default wrapper function that does nothing and returns a default constructed return object. Hence it is valid to call a `mem_fkn` which is in the null state. It will not affect the supplied object to the `invoke` call.

---

``` cpp
    template <class T, bool cnst, typename Signature> 
    class mem_fkn;
    template<class T, bool cnst, class R, class... Args>
    class mem_fkn<T, cnst, R(Args...)> 
```

| Type parameters | Description
| --- | ---
| **T** | Type for the object to call this member function on.
| **cnst** | *false* : Need to call on non const object. *true* : can call on const object.
| **Signature** | Signature for function call. Contain return type and argument types, similar to std::function.
| **R** | Return type from the called function.
| **Args** | Parameter pack with parameters for the function call.

### Member types

### Member functions

| Member function |  Description
| --- | ---
| `mem_fkn()` | Default construct a *mem_fkn*. Post-condition: `this->null() == true.`
| `bool null() const` | Return _true_ if the mem_fkn is in a null state.
| `void clear()`| Set *mem_fkn* to null state. Post-condition: `this->null() == true.`
| `explicit operator bool() const`| Return `!null()`

##### When cnst == false

| Member function |  Description
| --- | ---
| `R invoke(T& o, Args...)`| Call the stored wrapper on object _o_. Return the result from the call.
| `template<R (T::*mf)(Args...)> mem_fkn& set()` | Create a wrapper for *mf* member function and store the pointer. return `*this`
| `template<R (T::*mf)(Args...) const> mem_fkn& set_from_const()` | Create a wrapper for *mf* member function and store the pointer. return `*this`

##### When cnst == true

| Member function |  Description
| --- | ---
| `R invoke(T const& o, Args...)`| Call the stored wrapper on object _o_. Return the result from the call.
| `template<R (T::*mf)(Args...) const> mem_fkn& set()` | Create a wrapper for *mf* member function and store the pointer. return `*this`

### Static member functions

| Static function |  Description
| --- | ---
| `bool equal(mem_fkn const&, mem_fkn const&)` | Return _true_ if both arguments point to the same wrapper function or both are in null state.
| `bool less(mem_fkn const&, mem_fkn const&)` | Return _true_ if the _lhs_ wrapper function is < the _rhs_ wrapper function. The null state is < all stored wrappers.

##### When cnst == false

| Static function |  Description
| --- | ---
| `template<R (T::*mf)(Args...)> mem_fkn make()` | Create a wrapper for *mf* member function.  return a mem_fkn pointing to the wrapper.
| `template<R (T::*mf)(Args...) const> mem_fkn make_from_const()` | Create a wrapper for *mf* member function. Return a *mem_fkn* pointing to the wrapper.

##### When cnst == true

| Static function |  Description
| --- | ---
| `template<R (T::*mf)(Args...) const> mem_fkn make()` |Create a wrapper for *mf* member function.  return a *mem_fkn* pointing to the wrapper.

### Free functions, operator overloads.

| Static function |  Description
| --- | ---
| `bool operator==()` | Return true when considered *equal*.
| `bool operator!=()` | Return true when considered not *equal*.
| `bool operator<()` | Return true when considered *less(lhs,rhs)*.
| `bool operator<=()` | Return true when considered *less(lhs,rhs)* or *equal*.
| `bool operator>=()` | Return true when considered *less(rhs,lhs)* or *equal*.
| `bool operator>()` | Return true when considered *less(rhs,lhs)*.
