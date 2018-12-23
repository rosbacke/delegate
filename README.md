# delegate

Embedded friendly std::function alternative. Never heap alloc, no exceptions. Is
trivially_copyable.

## Overview

The 'delegate' is an include only embedded friendly alternative to std::function.
Main purpose is to store callable things such as free functions, member functions
and functors. Once stored, the delegate can be called without knowledge of the 
type of stored thing.

The delegate guarantees no heap allocation. In will never throw exceptions itself.
Intended use is as a general callback storage (think function pointer analog).
The price to pay is that the delegate only stores a pointer to referenced functor
objects or objects to call member functions on. 
The user needs to handle the lifetime of a refered object.

In addition the delegation object has a smaller footprint compared to common std::function 
implementations, using only 2 pointers (free function pointer and void pointer).
It avoids virtual dispatch internally. That result in small object code footprint and 
allows the optimizer to see through part of the call. This also means it fulfills the requirements
for being trivially_copyable, meaning it can safely be memcpy:ied between objects.
See e.g. [documentation on trivially copyable](https://en.cppreference.com/w/cpp/types/is_trivially_copyable).


## Quick start.
- clone the repo.
- Add include path <repo_root>/include
- In program '#include "delegate/delegate.hpp"'
- Use the 'delegate' template class.

Example use case:

    #include "delegate/delegate.hpp"
    
    struct Example {
	    static void staticFkn(int) {};
	    void memberFkn(int) {};
    };

    int main() 
    {
        Example e;
        delegate<void(int)> del;
    
        del.set<&Example::memberFkn>(e); // C++17
        del.set<Example, &Example::memberFkn>(e); // C++11,C++14
        del(123);
    
        del.set<Example::staticFkn>();
        del(456);
    }


## Design goals driving the design.

  * Should behave close to a normal function pointer.
    Small, efficient, no heap allocation, no exceptions.
    Does not keep track of refered object lifetime.
    
  * Implement type erasure. The delegate need to only contain
    enough type information to do a call. Additional type information
    should be hidden from the one instantiating a delegate object.

  * Support several call mechanisms. Supports:
    - Call to member function.
    - Call to free function.
    - Call to functors (by reference).
    - Call to stateless lambdas.
    - Extra call operations such as passing on stored void pointer.
    
  * It is always safe to call the delegate. In the null state a call will not
    do anything and return a default constructed return value.
    
  * Behave as a normal pointer type. Can be copied, compared for equality,
    called, and compared to nullptr.
    
  * Observe constness. Stored const objects references can only be called by
    by const member functions or via operator() const.
    
  * Delegate have two main way to set a function to be called:
    - set: Member function for setting a new value to an already existing delegate.
    - make: Static functions for constructing new delegate objects.
    Do note that constructors isn't used for function setup. See discussion later
    on why.
    
  * Be usable with C++11 while offering more functionality for later editions.
    - C++11 offer most functionality.
    - C++14 add constexpr on 'set' functions.
    - C++17 allow for simpler member function setup using template&lt;auto&gt;.
    
  * Discourage lifetime issues. Will not allow storing a reference to a temporary
    (rvalue-reference) to objects.
    
  * Be constexpr and exception friendly. As much as possible should be delared constexpr and noexcept. 
  
## mem_fkn (Prev. MemFkn)

A helper class to store a call to a member function without supplying an actual 
object to call on. It is a stand-alone class that can encapsulate a member function.
It has an 'invoke' member which accept an object and the stored member function is
called on that object, similar to std::invoke.
It can be stored, compared and copied around. Could e.g. be set up
in a std::map for lookup and later supplied an object to be called on.
mem_fkn uses a single free function pointer for storage and behaves as a standard value type.
It could be useful on its own to avoid having to deal with member pointer syntax in user code.

The main intended use is as an argument to delegate. A mem_fkn and an object can be given to 
the delegate class to tie an object to the member function and later the delegate
can be called in the normal way. mem_fkn offers a way to delay the combination of
the object and the member function name.
mem_fkn require more type information compared to delegate. In addition to call signature it 
require the type of the object to call on and a bool to signal if the functions stored
is a const method.

## Delegate examples

    #include "delegate/delegate.hpp"

    int testFkn(int x)
    {
       return x + 1;
    }

    int main()
    {
        // Only need to know enough type information to construct
        // the signature used during a call of the delegate. E.g.

        delegate<int(int)> del;
        int res = del(23);  // No delegate set, do default behavior.
        // res is now 0.

        if (!del)  // del return false if no fkn is stored. 
            del.set<testFkn>(); // Set a new function to be called.

        res = del(1); 
        // res is now 2.
    }

Base case for free functions are covered above. For member functions see below.

    #include "delegate/delegate.hpp"

    struct Test
    {
        int member(int x)
        { return x + 2; }
        int cmember(int x) const
        { return x + 3; }
    };

    int main()
    {
        delegate<int(int)> del;
        Test t;
        const Test ct;

        del.set<Test, &Test::member>(t);
        res = del(1); 
        // res is now 3.

        del.set<Test, &Test::cmember>(ct);
        res = del(1); 
        // res is now 4.
        
        // For C++17, we can deduce the object type, the following will work.
        del.set<&Test::member>(t);
        del.set<&Test::cmember>(ct);
        // ...
    }

For functors, the delegate expect the user to keep them alive.

    #include "delegate/delegate.hpp"

    struct TestF
    {
        int operator()(int x)
        { return x + 2; }
        int operator()(int x) const
        { return x + 3; }
    };

    int main()
    {
        delegate<int(int)> del;
        TestF t;
        const TestF ct;

        del.set(t);
        res = del(1); 
        // res is now 3.

        del.set(ct);
        res = del(1); 
        // res is now 4.
    }

Delegate can rely on stateless lambdas to convert to function pointers. In this
case we use runtime storage of the function pointer. Do note it will be less
easy for the compiler to optimize compared to a fully static setup.

    #include "delegate/delegate.hpp"

    int testFkn(int x)
    {
       return x + 5;
    }

    int main()
    {
        delegate<int(int)> del;

        del.set(testFkn);
        res = del(1); 
        // res is now 6.

        del.set(static_cast<int(*)(int)>([](int x) -> int { return x + 6; }));
        res = del(1);
        // res is now 7.

        // Or, use 'set_fkn' to get implicit conversion.
        del.set_fkn([](int x) -> int { return x + 7; });
        res = del(1);
        // res is now 8.
    }

For the runtime case there is no constructor template argument to give,
So delegate supports passing in a compatible free function pointer. This
will accept stateless lambdas also via function pointer conversion.
There is no lifetime issue with stateless lambdas.

    #include "delegate/delegate.hpp"

    int testFkn(int x)
    {
       return x + 5;
    }

    int main()
    {
        delegate<int(int)> del{testFkn};
        res = del(1); 
        // res is now 6.

		 auto del2 = delegate<int(int)>{ [](int x) -> int { return x + 6; } };
        res = del2(1);
        // res is now 7.
    }

Except the set function, one can use 'make' static functions to build delegate
objects directly. The most common one given below.

    #include "delegate/delegate.hpp"

    // .. types as before.

    int main()
    {
        using Del = delegate<int(int)>;
        Del del;

        Test t;  // For members.
        TestF functor; // For functor.

        del = Del::make<testFkn>();
        del = Del::make<Test, &Test::member>(t);
        del = Del::make<&Test::member>(t); // C++17

        del = Del::make(functor);

        del = Del::make(static_cast<int(*)(int)>([](int x) -> int 
                 { return x + 6; }));
        del = Del::make_fkn([](int x) -> int { return x + 6; }));
    }


## A note on skipping constructors.

Not using constructors to set up functions is due to how template 
arguments are used.
The delegate require class template arguments for the class and then
function address argument for the set/make function template.

Doing this with a constructor means the user needs to supply both
the class and constructor argument at the same time.
The syntax (if it is even possible) is not run of the mill.

Using a static member function it becomes easier:

    auto del = delegate<int(int)>::make<freeFkn>();

    // Pseudocode for constructor alternative:
    auto del = delegate<int(int)> /* some way to get <freeFkn> */ ();

The price to pay for normal static member function is the extra '::make'.
Another reason is that you can 'borrow' type information.

    auto del = delegate<int(int)>{};
    auto del2 = del.make<freeFkn>();

For normal runtime variables there is no constructor template type to
pass, so delegate do support passing in a runtime variable function pointer
of the right signature. It also allows implicit conversion from stateless lambdas
in this case.

### Alternative make_delegate free function.

In the spirit of smart pointer one could use 'make_delegate' to construct a 
delegate. However there is one thing to keep in mind. 
The address of a function is given by the combination of the signature and
the name. Hence make delegate would need to take both. E.g.

    auto del = make_delegate<int(int), freeFkn>();

Omitting the signature could work, but would be ambiguous as soon as the function is overloaded, severly affecting maintainability.
This will not offer extra functionality over using the 'make' functions
so there is currently no effort in supporting this.
