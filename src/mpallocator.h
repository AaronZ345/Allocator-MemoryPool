#ifndef MPALLOCATOR_H_
#define MPALLOCATOR_H_

#include <climits>
#include <cstddef>
#include <type_traits>
#include <new>
#include <cstring>
#include <cstdlib>


using namespace std;

template <typename _Ty, size_t BlockSize = 1048576>
class MPAllocator{
public:
    /* Member types */
    using _Not_user_specialized = void; // the type of void

    using value_type = _Ty; // the value type of T 

    typedef _Ty* pointer; // the pointer type of T
    typedef const _Ty* const_pointer; // the const pointer type of T

    typedef _Ty& reference; // the reference type of T
    typedef const _Ty& const_reference;  // the const reference type of T

    typedef size_t size_type; // the type of size_t
    typedef ptrdiff_t difference_type; // pointer subtraction type of result

    typedef true_type propagate_on_container_move_assignment;
    typedef true_type is_always_equal;
    //
    template <typename U>
    struct rebind {
        typedef MPAllocator<U> other;
    };

    /* Member functions。*/
    // Constructor
    MPAllocator() noexcept;

    // Destructor
    ~MPAllocator() noexcept;

    // Copy Constructor
    MPAllocator(MPAllocator& inputMP) noexcept;
    template <class U> MPAllocator(const MPAllocator<U>& inputMP) noexcept;

    // Get the address of the element
    pointer address(const_reference _Val) const noexcept;
    pointer address(reference _Val) const noexcept;

    // Allocate the memory space required for an object
    pointer allocate(size_type _Count);
    pointer allocate(size_type _Count, const void* hint);

    // Free the memory and return it to the memory pool
    void deallocate(pointer _Ptr, size_type _Count );

    // Get the maximum number of primes that can be achieved
    size_type max_size() const noexcept;

    // Constructs objects on the applied memory space
    template<class _Objty, class... _Types> void construct(_Objty *_Ptr, _Types&&... _Args);
    
    // Destructor object
    template<class _Uty> void destroy(_Uty *_Ptr);

private:

    // use Block to stroe each block we inquried to link the blocks in memory pool
    struct Block{
        Block* next;
    };
    typedef Block* BlockPtr;

    // use freeSequence to link the memory deallocated
    struct freeSequence{
        pointer startSlot;
        size_type slotNumber;
        freeSequence* next;
    };
    typedef freeSequence* freeSequencePtr;

    pointer currentSlot; // pointer to the first space for stroring value now
    pointer lastSlot; // pointer to the last space we can use to store value now
    BlockPtr currentBlock; // pointer to the block in memory pool we use now
    freeSequencePtr freeSlot; // pointer to freeSequence head
    
    //allocate blocksize block to memory pool
    void allocateBlock();
};

//operator overloading
template< class _Ty1, class _Ty2 > bool operator==( const MPAllocator<_Ty1>& lhs, const MPAllocator<_Ty2>& rhs );
template< class _Ty1, class _Ty2 > bool operator!=( const MPAllocator<_Ty1>& lhs, const MPAllocator<_Ty2>& rhs );

#include "mpallocator.tcc"

#endif
