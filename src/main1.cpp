#include <iostream>
#include <string>
#include "mpallocator.h"

int main()
{
    MPAllocator<int> a1;   // default allocator for ints
    int* a = a1.allocate(1);  // space for one int
    a1.construct(a, 7);       // construct the int
    std::cout << a[0] << '\n';
    a1.deallocate(a, 1);      // deallocate space for one int

    // default allocator for strings
    MPAllocator<std::string> a2;

    // same, but obtained by rebinding from the type of a1
    decltype(a1)::rebind<std::string>::other a2_1;

    // same, but obtained by rebinding from the type of a1 via allocator_traits
    std::allocator_traits<decltype(a1)>::rebind_alloc<std::string> a2_2;

    std::string* s = a2.allocate(2); // space for 2 strings

    a2.construct(s, "foo");
    a2.construct(s + 1, "bar");

    std::cout << s[0] << ' ' << s[1] << '\n';

    a2.destroy(s);
    a2.destroy(s + 1);
    a2.deallocate(s, 2);
}
