#pragma once

#include <assert.h>

template<typename T>
struct LinkedListNode;

template<typename T>
class LinkedList
{
private:
    LinkedListNode<T>* m_first;
    LinkedListNode<T>* m_last;
    int m_count;

public:
    LinkedList()
    {
        this->m_count = 0;
        this->m_first = NULL;
        this->m_last = NULL;
    }
    inline LinkedListNode<T>*& First()
    {
        return this->m_first;
    }
    inline LinkedListNode<T>*& Last()
    {
        return this->m_last;
    }
    inline int Count()
    {
        return this->m_count;
    }
    inline bool AddFirst(LinkedListNode<T>* value)
    {
        if (value == NULL)
        {
            return false;
        }
        value->LinkedList_ = NULL;
        value->Next = NULL;
        value->Previous = NULL;

        if (this->m_last == NULL)
        {
            this->m_last = value;
            this->m_first = value;
            this->m_count = 0;
        }
        else
        {
            LinkedListNode<T>* current = this->m_first;
            value->Next = current;
            current->Previous = value;
            this->m_first = value;
        }
        this->m_count++;
        value->LinkedList_ = this;
        return true;
    }
    inline bool AddLast(LinkedListNode<T>* node)
    {
        if (node == NULL)
        {
            return false;
        }
        node->LinkedList_ = NULL;
        node->Next = NULL;
        node->Previous = NULL;

        if (this->m_last == NULL)
        {
            this->m_first = node;
            this->m_last = node;
            this->m_count = 0;

            this->m_count++;
            node->LinkedList_ = this;
            return true;
        }
        else
        {
            return this->AddAfter(this->m_last, node);
        }
    }
    inline bool AddAfter(LinkedListNode<T>* node, LinkedListNode<T>* value)
    {
        if (node == NULL || value == NULL)
        {
            return false;
        }
        value->LinkedList_ = NULL;
        value->Next = NULL;
        value->Previous = NULL;
        LinkedListNode<T>* current = node->Next;
        node->Next = value;
        if (current != NULL)
        {
            current->Previous = value;
        }
        value->Previous = node;
        value->Next = current;
        if (node == this->m_last)
        {
            this->m_last = value;
        }
        this->m_count++;
        value->LinkedList_ = this;
        return true;
    }
    inline bool AddBefore(LinkedListNode<T>* node, LinkedListNode<T>* value)
    {
        if (node == NULL || value == NULL)
        {
            return false;
        }
        value->LinkedList_ = NULL;
        value->Next = NULL;
        value->Previous = NULL;
        LinkedListNode<T> current = node->Previous;
        if (current == NULL)
        {
            return this->AddFirst(value);
        }
        current.Next = value;
        node->Previous = value;
        value->Next = node;
        value->Previous = current;
        if (node == this->m_first)
        {
            this->m_first = value;
        }
        this->m_count++;
        value->LinkedList_ = this;
        return true;
    }
    inline bool RemoveFirst()
    {
        LinkedListNode<T>* first = this->m_first;
        if (first == NULL)
        {
            return false;
        }
        LinkedListNode<T>* current = first->Next;
        first->Previous = NULL;
        first->LinkedList_ = NULL;
        first->Next = NULL;
        if (current != NULL)
        {
            current->Previous = NULL;
        }
        this->m_count--;
        if (this->m_count <= 0)
        {
            this->m_count = 0;
            this->m_first = NULL;
            this->m_last = NULL;
            current = NULL;
        }
        this->m_first = current;
        return true;
    }
    inline bool RemoveLast()
    {
        LinkedListNode<T>* last = this->m_last;
        if (last == NULL)
        {
            return false;
        }
        LinkedListNode<T>* current = last->Previous;
        last->Previous = NULL;
        last->LinkedList_ = NULL;
        last->Next = NULL;
        if (current != NULL)
        {
            current->Next = NULL;
        }
        this->m_count--;
        if (this->m_count <= 0)
        {
            this->m_count = 0;
            this->m_first = NULL;
            this->m_last = NULL;
            current = NULL;
        }
        this->m_last = current;
        return true;
    }
    inline bool Remove(LinkedListNode<T>* node)
    {
        if (node == NULL)
        {
            return false;
        }
        if (node == this->m_first)
        {
            return this->RemoveFirst();
        }
        if (node == this->m_last)
        {
            return this->RemoveLast();
        }
        LinkedListNode<T>* previous = node->Previous;
        LinkedListNode<T>* next = node->Next;
        previous->Next = next;
        next->Previous = previous;
        this->m_count--;
        if (this->m_count <= 0)
        {
            this->m_count = 0;
            this->m_first = NULL;
            this->m_last = NULL;
        }
        node->Next = NULL;
        node->Previous = NULL;
        node->LinkedList_ = NULL;
        return true;
    }
    inline LinkedListNode<T>* Find(T value)
    {
        LinkedListNode<T>* i = this->m_first;
        while (i != NULL)
        {
            if (i->Value == value)
            {
                return i;
            }
            i = i->Next;
        }
        return NULL;
    }
    inline void Clear()
    {
        LinkedListNode<T>* i = this->m_first;
        while (i != NULL)
        {
            LinkedListNode<T>* j = i->Next;
            {
                i->LinkedList_ = NULL;
                i->Next = NULL;
                i->Previous = NULL;
            }
            i = j;
        }
        this->m_first = NULL;
        this->m_count = 0;
        this->m_last = NULL;
    }
};

template<typename T>
struct LinkedListNode
{
public:
    LinkedListNode<T>* Previous;
    LinkedListNode<T>* Next;
    T Value;
    LinkedList<T>* LinkedList_;
};