# delegate
Embedded friendly std::function alternative. Never heap alloc, no exceptions

# Overview
The 'delegate' is an include only embedded friendly alternative to std::function.
It guarantees no heap allocation. In will never throw exceptions itself.
Intended use is as a general callback storage (think function pointer analog).
The price to pay is the user needs to handle the lifetime of a state carrying
functor.
In addition the delegation object has a smaller footprint compared to common std::function implementations. It also avoids virtual dispatch internally. That result in small object code footprint and allows the optimizer to see through part of the call.


## Quick start.
- clone the repo.
- Add include path <repo_root>/include
- In program '#include "delegate/delegate.hpp"'
- Use the 'delegate' template class.

Example use case:

    #include "delegate/delegtate.h"
    
    struct Example {
	    static void staticFkn(int) {};
	    void memberFkn(int) {};
    };

    int main() 
    {
	    Example e;
        delegate<void(int)> cb;
    
        cb.set<Example, &Example::memberFkn>(e);
        cb(123);
    
        cb.set<Example::staticFkn>();
        cb(456);
    }


