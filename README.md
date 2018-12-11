# delegate
Embedded friendly std::function alternative. Never heap alloc, no exceptions

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
allows the optimizer to see through part of the call.

## Quick start.
- clone the repo.
- Add include path <repo_root>/include
- In program '#include "delegate/delegate.hpp"'
- Use the 'delegate' template class.

Example use case:

    #include "delegate/delegtate.hpp"
    
    struct Example {
	    static void staticFkn(int) {};
	    void memberFkn(int) {};
    };

    int main() 
    {
	    Example e;
        delegate<void(int)> cb;
    
        cb.set<&Example::memberFkn>(e); // C++17
        cb.set<Example, &Example::memberFkn>(e); // C++11,C++14
        cb(123);
    
        cb.set<Example::staticFkn>();
        cb(456);
    }


## Design goals driving the design.

  * Should behave close to a normal function pointer.
    Small, effiecent, no heap allocation, no exceptions.
    Does not keep track of refered object lifetime.
    
  * Implement type erasure. The delegate need to only contain
    enough type information to do a call. Additional type information
    should be hidden from the one instantiationg a delegate object.
    Do note that the MemFkn have more type information since it 
    tracks constness of the member function.
    
  * It is always safe to call the delegate. In the null state a call will not
    do anything and return a default constructed return value.
    
  * Behave as a normal pointer type. Can be copied, compared for equality,
    called, and compared to nullptr.
    
  * Observe constness. Stored const objects references can only be called by
    by const member functions or via operator() const.
    
  * Having 2 different ways to change values. Static functions 'make' to construct
    a new delegate or member function 'set' to change a delegate in-place.
    
  * Be usable with C++11 while offering more functionality for later editions.
    Most of it works in C++11. C++14 adds constexpr on 'set' functions. C++17 adds
    simplified member function pointer setup using template<auto>.
    
  * Discourage lifetime issues. Will not allow storing a reference to a temporary
    (rvalue-reference) to objects.
    
  * Be constexpr friendly. As much as possible should be delared constexpr. 
  
## MemFkn

A helper class to store a call to a member function without supplying an actual 
object to call on. The only useful thing MemFkn can do is later to be an argument
to a delegate together with an object. 
It does allow encapsulating a member function pointer and treat it as any other 
value type. It can be stored, compared and copied around. Could e.g. be set up
in a std::map for lookup and later supplied an object to be called on.

 