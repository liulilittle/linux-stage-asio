#pragma once

template<typename TPosition, typename TValue>
class LinkedList_Fix;

template<typename TPosition, typename TValue>
struct LinkedListNode_Fix
{
public:
    TPosition                                       Previous;
    TPosition                                       Next;
    TValue                                          Value;

public:
    inline void                                     Reset()
    {
        this->Previous = 0;
        this->Next = 0;
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   GetNextNode(LinkedList_Fix<TPosition, TValue>* linkedlist)
    {
        if (linkedlist == NULL)
        {
            return NULL;
        }
        return linkedlist->GetNode(this->Next);
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   GetPreviousNode(LinkedList_Fix<TPosition, TValue>* linkedlist)
    {
        if (linkedlist == NULL)
        {
            return NULL;
        }
        return linkedlist->GetNode(this->Previous);
    }
};

template<typename TPosition, typename TValue>
class LinkedList_Fix
{
private:
    TPosition                                       m_First;
    TPosition                                       m_Last;
    int                                             m_Count;
    int                                             m_Capacity;

    inline LinkedListNode_Fix<TPosition, TValue>*   GetFirstNode()
    {
        char* fst = ((char*)this) + sizeof(*this);
        return (LinkedListNode_Fix<TPosition, TValue>*)fst;
    }

public:
    inline LinkedListNode_Fix<TPosition, TValue>*   GetNode(TPosition position = 1)
    {
        if (position > 0)
        {
            if (this->m_Capacity > 0 && position > this->m_Capacity)
            {
                return 0;
            }
            return (this->GetFirstNode() + (position - 1));
        }
        return NULL;
    }
    inline TPosition                                GetPosition(LinkedListNode_Fix<TPosition, TValue>* node)
    {
        if (node == NULL)
        {
            return 0;
        }
        TPosition position = (TPosition)(node - this->GetFirstNode()) + 1;
        if (position < 1)
        {
            return 0;
        }
        if (this->m_Capacity != 0)
        {
            if (position > this->m_Capacity)
            {
                return 0;
            }
        }
        return position;
    }

public:
    inline void                                     Initialize(int capacity = 0)
    {
        this->m_Capacity = capacity;
        this->Clear();
    }

public:
    inline LinkedListNode_Fix<TPosition, TValue>*   First()
    {
        return this->GetNode(this->m_First);
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   Last()
    {
        return this->GetNode(this->m_Last);
    }
    inline int                                      Count()
    {
        return this->m_Count;
    }
    inline bool                                     AddFirst(LinkedListNode_Fix<TPosition, TValue>* node)
    {
        if (node == NULL)
        {
            return false;
        }

        node->Next = 0;
        node->Previous = 0;

        if (this->m_First == 0 || this->m_Last == 0)
        {
            this->m_Last = this->GetPosition(node);
            this->m_First = this->GetPosition(node);
            this->m_Count = 0;
        }
        else
        {
            LinkedListNode_Fix<TPosition, TValue>* current = this->GetNode(this->m_First);
            node->Next = this->m_First;
            current->Previous = this->GetPosition(node);
            this->m_First = this->GetPosition(node);
        }
        this->m_Count++;

        return true;
    }
    inline bool                                     AddLast(LinkedListNode_Fix<TPosition, TValue>* node)
    {
        if (node == NULL)
        {
            return false;
        }

        node->Next = 0;
        node->Previous = 0;

        if (this->m_First == 0 || this->m_Last == 0)
        {
            this->m_First = this->GetPosition(node);
            this->m_Last = this->GetPosition(node);
            this->m_Count = 0;

            this->m_Count++;

            return true;
        }
        else
        {
            return this->AddAfter(this->GetNode(this->m_Last), node);
        }
    }

    inline bool                                     AddAfter(LinkedListNode_Fix<TPosition, TValue>* node, LinkedListNode_Fix<TPosition, TValue>* value)
    {
        if (node == NULL || value == NULL)
        {
            return false;
        }

        value->Next = 0;
        value->Previous = 0;

        LinkedListNode_Fix<TPosition, TValue>* current = this->GetNode(node->Next);
        node->Next = this->GetPosition(value);

        if (current != NULL)
        {
            current->Previous = this->GetPosition(value);
        }
        value->Previous = this->GetPosition(node);
        value->Next = this->GetPosition(current);

        if (this->GetPosition(node) == this->m_Last)
        {
            this->m_Last = this->GetPosition(value);
        }
        this->m_Count++;

        return true;
    }
    inline bool                                     AddBefore(LinkedListNode_Fix<TPosition, TValue>* node, LinkedListNode_Fix<TPosition, TValue>* value)
    {
        if (node == NULL || value == NULL)
        {
            return false;
        }

        value->Next = 0;
        value->Previous = 0;

        LinkedListNode_Fix<TPosition, TValue>* current = this->GetNode(node->Previous);
        if (current == NULL)
        {
            return this->AddFirst(value);
        }

        current->Next = this->GetPosition(value);
        node->Previous = this->GetPosition(value);
        value->Next = this->GetPosition(node);
        value->Previous = this->GetPosition(current);

        if (this->GetPosition(node) == this->m_First)
        {
            this->m_First = this->GetPosition(value);
        }
        this->m_Count++;

        return true;
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   RemoveFirst()
    {
        LinkedListNode_Fix<TPosition, TValue>* first = this->GetNode(this->m_First);
        if (first == NULL)
        {
            return NULL;
        }
        LinkedListNode_Fix<TPosition, TValue>* current = this->GetNode(first->Next);

        first->Previous = 0;
        first->Next = 0;

        if (current != NULL)
        {
            current->Previous = 0;
        }

        this->m_Count--;
        if (this->m_Count <= 0)
        {
            this->m_Count = 0;
            this->m_First = 0;
            this->m_Last = 0;
            current = NULL;
        }

        this->m_First = this->GetPosition(current);

        return first;
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   RemoveLast()
    {
        LinkedListNode_Fix<TPosition, TValue>* last = this->GetNode(this->m_Last);
        if (last == NULL)
        {
            return NULL;
        }
        LinkedListNode_Fix<TPosition, TValue>* current = this->GetNode(last->Previous);
        last->Previous = 0;
        last->Next = 0;

        if (current != NULL)
        {
            current->Next = 0;
        }

        this->m_Count--;
        if (this->m_Count <= 0)
        {
            this->m_Count = 0;
            this->m_First = 0;
            this->m_Last = 0;
            current = NULL;
        }

        this->m_Last = this->GetPosition(current);

        return last;
    }
    inline bool                                     Remove(LinkedListNode_Fix<TPosition, TValue>* node)
    {
        if (node == NULL)
        {
            return false;
        }
        if (this->GetPosition(node) == this->m_First)
        {
            return this->RemoveFirst();
        }
        if (this->GetPosition(node) == this->m_Last)
        {
            return this->RemoveLast();
        }

        LinkedListNode_Fix<TPosition, TValue>* previous = this->GetNode(node->Previous);
        LinkedListNode_Fix<TPosition, TValue>* next = this->GetNode(node->Next);
        previous->Next = node->Next;
        next->Previous = node->Previous;

        this->m_Count--;
        if (this->m_Count <= 0)
        {
            this->m_Count = 0;
            this->m_First = 0;
            this->m_Last = 0;
        }
        node->Next = 0;
        node->Previous = 0;

        return true;
    }
    inline LinkedListNode_Fix<TPosition, TValue>*   Find(const TValue& value)
    {
        LinkedListNode_Fix<TPosition, TValue>* next = this->m_First;
        int count = 0;
        while (next != NULL && count++ < this->m_Count)
        {
            if (next->Value == value)
            {
                return next;
            }
            next = this->GetNode(next->Next);
        }
        return NULL;
    }
    inline void                                     Clear()
    {
        this->m_Last = 0;
        this->m_First = 0;
        this->m_Count = 0;
    }
    inline int&                                     Capacity()
    {
        return this->m_Capacity;
    }
    inline bool                                     Invalid(TPosition position)
    {
        return this->GetNode(position) == NULL;
    }
    inline bool                                     Invalid(LinkedListNode_Fix<TPosition, TValue>* node)
    {
        return !(this->GetPosition(node) > 0);
    }
public:
    inline bool                                     AddFirst(TPosition node)
    {
        return this->AddFirst(this->GetNode(node));
    }
    inline bool                                     AddLast(TPosition node)
    {
        return this->AddLast(this->GetNode(node));
    }
    inline bool                                     Remove(TPosition node)
    {
        return this->Remove(this->GetNode(node));
    }
    inline bool                                     AddAfter(TPosition node, TPosition value)
    {
        return this->AddAfter(this->GetNode(node), this->GetNode(value));
    }
    inline bool                                     AddBefore(TPosition node, TPosition value)
    {
        return this->AddBefore(this->GetNode(node), this->GetNode(value));
    }
    inline bool                                     AddAfter(TPosition node, LinkedListNode_Fix<TPosition, TValue>* value)
    {
        return this->AddAfter(this->GetNode(node), value);
    }
    inline bool                                     AddBefore(TPosition node, LinkedListNode_Fix<TPosition, TValue>* value)
    {
        return this->AddBefore(this->GetNode(node), value);
    }
    inline bool                                     AddAfter(LinkedListNode_Fix<TPosition, TValue>* node, TPosition value)
    {
        return this->AddAfter(node, this->GetNode(value));
    }
    inline bool                                     AddBefore(LinkedListNode_Fix<TPosition, TValue>* node, TPosition value)
    {
        return this->AddBefore(node, this->GetNode(value));
    }
    inline static bool                              Invalid(LinkedList_Fix<TPosition, TValue>* linkedlist, TPosition position)
    {
        if (linkedlist == NULL)
        {
            return true;
        }
        return linkedlist->Invalid(position);
    }
    inline static bool                              Invalid(LinkedList_Fix<TPosition, TValue>* linkedlist, LinkedListNode_Fix<TPosition, TValue>* node)
    {
        if (linkedlist == NULL || node == NULL)
        {
            return true;
        }
        return linkedlist->Invalid(node);
    }
};