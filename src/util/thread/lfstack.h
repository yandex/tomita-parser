#pragma once

#include <util/system/atomic.h>


//////////////////////////////
// lock free lifo stack
template <class T>
class TLockFreeStack
{
    struct TNode
    {
        T Value;
        TNode *Next;

        TNode() {
        }
        TNode(const T &val)
            : Value(val)
            , Next(0)
        {
        }
    };

    TNode *volatile Head;
    TNode *volatile FreePtr;
    TAtomic DequeueCount;

    void TryToFreeMemory()
    {
        TNode *current = FreePtr;
        if (!current)
            return;
        if (AtomicAdd(DequeueCount, 0) == 1) {
            // node current is in free list, we are the last thread so try to cleanup
            if (AtomicCas(&FreePtr, (TNode*)0, current))
                EraseList(current);
        }
    }
    void EraseList(TNode *volatile p)
    {
        while (p) {
            TNode *next = p->Next;
            delete p;
            p = next;
        }
    }
    void EnqueueImpl(TNode* volatile head, TNode* volatile tail)
    {
        for (;;) {
            tail->Next = Head;
            if (AtomicCas(&Head, head, tail->Next))
                break;
        }
    }
    TLockFreeStack(const TLockFreeStack&) {
    }
    void operator=(const TLockFreeStack&) {
    }
public:
    TLockFreeStack()
        : Head(0)
        , FreePtr(0)
        , DequeueCount(0)
    {
    }
    ~TLockFreeStack()
    {
        EraseList(Head);
        EraseList(FreePtr);
    }
    void Enqueue(const T& t)
    {
        TNode *volatile node = new TNode(t);
        EnqueueImpl(node, node);
    }
    template <typename TCollection>
    void EnqueueAll(const TCollection& data)
    {
        EnqueueAll(data.begin(), data.end());
    }
    template <typename TIter>
    void EnqueueAll(TIter dataBegin, TIter dataEnd)
    {
        if (dataBegin == dataEnd) {
            return;
        }
        TIter i = dataBegin;
        TNode *volatile node = new TNode(*i);
        TNode *volatile tail = node;

        for (++i; i != dataEnd; ++i) {
            TNode* nextNode = node;
            node = new TNode(*i);
            node->Next = nextNode;
        }
        EnqueueImpl(node, tail);
    }
    bool Dequeue(T *res)
    {
        AtomicAdd(DequeueCount, 1);
        for (TNode *current = Head; current; current = Head) {
            if (AtomicCas(&Head, current->Next, current)) {
                *res = current->Value;
                // delete current; // ABA problem
                // even more complex node deletion
                TryToFreeMemory();
                if (AtomicAdd(DequeueCount, -1) == 0) {
                    // no other Dequeue()s, can safely reclaim memory
                    delete current;
                } else {
                    // Dequeue()s in progress, put node to free list
                    for (;;) {
                        current->Next = FreePtr;
                        if (AtomicCas(&FreePtr, current, current->Next))
                            break;
                    }
                }
                return true;
            }
        }
        TryToFreeMemory();
        AtomicAdd(DequeueCount, -1);
        return false;
    }
    // add all elements to *res
    // elements are returned in order of dequeue (top to bottom; see example in unittest)
    template <typename TCollection>
    void DequeueAll(TCollection *res)
    {
        AtomicAdd(DequeueCount, 1);
        for (TNode *current = Head; current; current = Head) {
            if (AtomicCas(&Head, (TNode*)0, current)) {
                for (TNode *x = current; x;) {
                    res->push_back(x->Value);
                    x = x->Next;
                }
                // EraseList(current); // ABA problem
                // even more complex node deletion
                TryToFreeMemory();
                if (AtomicAdd(DequeueCount, -1) == 0) {
                    // no other Dequeue()s, can safely reclaim memory
                    EraseList(current);
                } else {
                    // Dequeue()s in progress, add nodes list to free list
                    TNode *currentLast = current;
                    while (currentLast->Next) {
                        currentLast= currentLast->Next;
                    }
                    for (;;) {
                        currentLast->Next = FreePtr;
                        if (AtomicCas(&FreePtr, current, currentLast->Next))
                            break;
                    }
                }
                return;
            }
        }
        TryToFreeMemory();
        AtomicAdd(DequeueCount, -1);
    }
    bool DequeueSingleConsumer(T *res)
    {
        for (TNode *current = Head; current; current = Head) {
            if (AtomicCas(&Head, current->Next, current)) {
                *res = current->Value;
                delete current; // with single consumer thread ABA does not happen
                return true;
            }
        }
        return false;
    }
    // add all elements to *res
    // elements are returned in order of dequeue (top to bottom; see example in unittest)
    template <typename TCollection>
    void DequeueAllSingleConsumer(TCollection *res)
    {
        for (TNode *current = Head; current; current = Head) {
            if (AtomicCas(&Head, (TNode*)0, current)) {
                for (TNode *x = current; x;) {
                    res->push_back(x->Value);
                    x = x->Next;
                }
                EraseList(current); // with single consumer thread ABA does not happen
                return;
            }
        }
    }
    bool IsEmpty()
    {
        AtomicAdd(DequeueCount, 0); // mem barrier
        return Head == 0; // access is not atomic, so result is approximate
    }
};
