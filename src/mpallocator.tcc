#ifndef MPALLOCATOR_TCC_
#define MPALLOCATOR_TCC_

// Constructor
template <typename _Ty, size_t BlockSize>
MPAllocator<_Ty, BlockSize>::MPAllocator() noexcept {
    //inistialize the pointers to null
    currentBlock = nullptr;
    currentSlot = nullptr;
    lastSlot = nullptr;
    freeSlot = nullptr;
}

// Destructor
template <typename _Ty, size_t BlockSize>
MPAllocator<_Ty, BlockSize>::~MPAllocator() noexcept {
    BlockPtr currP = currentBlock;

    //delete the blocks we have operator newed from memory
    if(currP != nullptr) {
        BlockPtr nextP = currP->next;
        for (; currP != nullptr; currP = nextP) {
            nextP = currP->next;
            operator delete(reinterpret_cast<void *>(currP)); //use void* since no dtor are needed
        }
    }

    //delete the space we have newed for freesequence
    if(freeSlot != nullptr) {
        freeSequencePtr nextFP = freeSlot->next;
        for (; freeSlot != nullptr; freeSlot = nextFP) {
            nextFP = freeSlot->next;
            operator delete(reinterpret_cast<void *>(freeSlot)); //use void* since no dtor are needed
        }
    }
}

// Copy Constructor
template <typename _Ty, size_t BlockSize>
MPAllocator<_Ty, BlockSize>::MPAllocator(MPAllocator &inputMP) noexcept {
    currentBlock = inputMP.currentBlock;
    currentSlot = inputMP.currentSlot;
    lastSlot = inputMP.lastSlot;
    freeSlot = inputMP.freeSlot;
    inputMP.currentBlock = nullptr; //delete the input's pointer to avoid delete pointer duplicated
}

template <typename _Ty, size_t BlockSize>
template<class U>
MPAllocator<_Ty, BlockSize>::MPAllocator(const MPAllocator<U>& inputMP) noexcept:
        MPAllocator(){
    //do nothing cause c++17 STL don't do anything, too
}

// Get the address of the element
template <typename _Ty, size_t BlockSize>
inline typename MPAllocator<_Ty, BlockSize>::pointer
MPAllocator<_Ty, BlockSize>::address(const_reference _Val) const noexcept {
    return &_Val;
}

template <typename _Ty, size_t BlockSize>
inline typename MPAllocator<_Ty, BlockSize>::pointer
MPAllocator<_Ty, BlockSize>::address(reference _Val) const noexcept {
    return &_Val;
}

// Allocate the memory space required for an object
template <typename _Ty, size_t BlockSize>
inline typename MPAllocator<_Ty, BlockSize>::pointer
MPAllocator<_Ty, BlockSize>::allocate(size_type _Count) {
    if(_Count == 0) return nullptr; //test for require 0 count size

    size_type needSize = _Count * sizeof(value_type); //calculate the required size by count*size

    //if needsize is too large then we return a operator new size
    if(needSize > BlockSize) {
        size_type requireSize = needSize + sizeof(BlockPtr) + sizeof(value_type);
        BlockPtr newBlock = reinterpret_cast<BlockPtr>(operator new (requireSize));
        newBlock->next = currentBlock->next;
        currentBlock->next = newBlock;
        BlockPtr body = reinterpret_cast<BlockPtr>(reinterpret_cast<size_type>(newBlock) + sizeof(BlockPtr));
        uintptr_t result = reinterpret_cast<uintptr_t>(body);
        size_type bodyPadding = ((alignof(value_type) - result) % alignof(value_type));
        return reinterpret_cast<pointer>(newBlock + bodyPadding);
    }

    //if we don't have freeslot memory, we need to use block in memorypool
    if(freeSlot == nullptr) {
        if (lastSlot - currentSlot < _Count) allocateBlock(); //if block don't have enough memory, allocate for new memory
        pointer returnP = reinterpret_cast<pointer>(currentSlot);
        currentSlot += _Count;
        return returnP; //return enough space and then the remaining space is reduced, so we add the currentslot with 1
    }

        //if we have freeslot memory, then we can use it
    else{
        freeSequencePtr curr=freeSlot;
        freeSequencePtr last=freeSlot;

        //traverse the freesequence if we can find a enough space for requirement
        for(; curr != nullptr; last=curr, curr = curr->next) {

            //if we find a freeslot node whose space is larger than reqirement
            if (curr->slotNumber > _Count) {
                curr->slotNumber -= _Count; //reduce the require space
                return curr->startSlot + curr->slotNumber; //return the reqire space
            }

            //if we find a freeslot node whose apce is just equal to requirement, we drop it
            if (curr->slotNumber == _Count) {
                last->next = curr->next;
                pointer currSlot = curr->startSlot;
                //if curr point to the head node of freeslot, then move it back
                if(curr == freeSlot)
                    freeSlot = curr->next;
                free(curr);
                return currSlot;
            }
        }

        //if freeslot have no suitable node, use block in memory pool
        if (lastSlot - currentSlot < _Count) allocateBlock();
        pointer returnP2 = reinterpret_cast<pointer>(currentSlot);
        currentSlot += _Count;
        return returnP2; //return enough space and then the remaining space is reduced, so we add the currentslot with 1
    }
}

template <typename _Ty, size_t BlockSize>
inline typename MPAllocator<_Ty, BlockSize>::pointer
MPAllocator<_Ty, BlockSize>::allocate(size_type _Count, const void* hint) {
    return allocate(_Count); //just ignore the hint since c++17 STL also ignore it
}

// Free the memory and return it to the memory pool
template <typename _Ty, size_t BlockSize>
inline void
MPAllocator<_Ty, BlockSize>::deallocate(pointer _Ptr, size_type _Count) {
    //if count is 0 or pointer is null we don't need to deallocate anything
    if(_Count == 0 || _Ptr == nullptr) return;

    //deallocate the space into our freesquence
    freeSequencePtr tmp = new freeSequence;
    tmp->startSlot=_Ptr;
    tmp->slotNumber = _Count;
    tmp->next = nullptr;

    //if we don't have freesequence yet then we add the node as the first node
    if(freeSlot == nullptr) freeSlot=tmp;

        //if we only have one freesequence node yet, we sort them with start pointer from small to big
    else if(freeSlot->next == nullptr) {
        if(freeSlot->startSlot > tmp->startSlot) {
            tmp->next = freeSlot;
            freeSlot = tmp;
        }
        else freeSlot->next = tmp;
    }

        //if we have many nodes in freesequence yet, we need to sort them and find if the consecutive twos can merge to one
    else{
        //if the node should be inserted as the head node
        if(tmp->startSlot<freeSlot->startSlot){
            tmp->next = freeSlot;
            freeSlot = tmp;
            return;
        }
        freeSequencePtr curr=freeSlot->next;
        freeSequencePtr last=freeSlot;

        //traverse the freesequencce and have a insert sort
        for(; curr != nullptr; last = curr, curr = curr->next) {

            //find the place the node should be inserted in
            if(curr->startSlot>tmp->startSlot && last->startSlot < tmp->startSlot) {

                //if the last one can connected with the current one
                if(last->startSlot+last->slotNumber == tmp->startSlot) {
                    last->slotNumber += tmp->slotNumber;

                    //if the current one also can connect with the next one
                    if(tmp->startSlot+tmp->slotNumber == curr->startSlot) {
                        last->slotNumber += curr->slotNumber;
                        last->next = curr->next;
                        free(curr);
                    }
                    free(tmp);
                }

                    //if the curren one can only connect with the next one
                else if(tmp->startSlot+tmp->slotNumber == curr->startSlot) {
                    tmp->slotNumber += curr->slotNumber;
                    last->next = tmp;
                    tmp->next = curr->next;
                    free(curr);
                }

                    //if none of the nodes can be merged
                else {
                    last->next = tmp;
                    tmp->next = curr;
                }

                return;
            }
        }
        
        //if the node should be inserted as the last node
        last->next = tmp;
        return;
    }
}

// Get the maximum number of primes that can be achieved
template <typename _Ty, size_t BlockSize>
inline typename MPAllocator<_Ty, BlockSize>::size_type
MPAllocator<_Ty, BlockSize>::max_size() const noexcept {
    //code in c++17 STL like way
    size_type maxBlocks = -1 / BlockSize;
    return (BlockSize - sizeof(BlockPtr)) / sizeof(value_type) * maxBlocks; //except for the first pointer for link the blocks in memory pool space, the remaining space can use for allocating
}

// Constructs objects on the applied memory space
template <typename _Ty, size_t BlockSize>
template<class _Objty, class... _Types>
inline void
MPAllocator<_Ty, BlockSize>::construct(_Objty *_Ptr, _Types&&... _Args) {
    new (_Ptr) _Objty(forward<_Types>(_Args)...); //construct it with the type and paramater input
}

// Destructor object
template <typename _Ty, size_t BlockSize>
template<class _Uty>
inline void
MPAllocator<_Ty, BlockSize>::destroy(_Uty *_Ptr) {
    _Ptr->~_Uty(); //implement the dtor of the pointer input to destroy the object
}

//allocate blocksize block to memory pool
template <typename _Ty, size_t BlockSize>
inline void
MPAllocator<_Ty, BlockSize>::allocateBlock() {
    //require a new blocksize block into memorypool and link it with other memorypool blocks
    BlockPtr newBlock = reinterpret_cast<BlockPtr>(operator new(BlockSize));
    newBlock->next = currentBlock;
    currentBlock = newBlock;

    //calculate the alignment of the type for template
    BlockPtr body = reinterpret_cast<BlockPtr>(reinterpret_cast<size_type>(newBlock) + sizeof(BlockPtr));
    uintptr_t result = reinterpret_cast<uintptr_t>(body);
    size_type bodyPadding = ((alignof(value_type) - result) % alignof(value_type));

    //align the space in memory pool current block with the backward alignment
    currentSlot = reinterpret_cast<pointer>(body + bodyPadding); //point to the first spcace we can use now
    lastSlot = reinterpret_cast<pointer>(newBlock + BlockSize - sizeof(value_type) + 1); //point to the last space we can use now
}

//operator overloading
template< class _Ty1, class _Ty2 >
inline bool
operator==( const MPAllocator<_Ty1>& lhs, const MPAllocator<_Ty2>& rhs ) {
    return true; //just return true since c++17 STL also do it only
}

template< class _Ty1, class _Ty2 >
inline bool
operator!=( const MPAllocator<_Ty1>& lhs, const MPAllocator<_Ty2>& rhs ) {
    return false; //just return flase since c++17 STL also do it only
}

#endif
