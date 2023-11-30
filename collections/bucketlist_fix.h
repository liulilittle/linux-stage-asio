#pragma once

#include "linkedlist.h"
#include <vector>

template<typename TPosition, typename TValue>
class BucketList_Fix // Ͱ
{
private:
    LinkedList_Fix<TPosition, TValue>*              m_linkedlist;
    TPosition*                                      m_frees;
    TPosition*                                      m_freestack;         // ָ���ͷŽڵ�Ļ���ջ
    LinkedListNode_Fix<TPosition, TValue>*          m_nodes;
    volatile int                                    m_count;
    volatile int                                    m_capacity;

    inline LinkedListNode_Fix<TPosition, TValue>*   GetDataNodeAddress(int capacity) // ��ȡ���ݽڵ�ĵ�ַ
    {
        return this->GetLinkedListAddress(capacity)->GetNode();
    }
    inline TPosition*                               GetFreeNodeAddress(int capacity)
    {
        int ofs_ = sizeof(BucketList_Fix<TPosition, TValue>);
        return (TPosition*)(((char*)this) + ofs_);
    }
    inline LinkedList_Fix<TPosition, TValue>*       GetLinkedListAddress(int capacity)
    {
        int ofs_ = sizeof(BucketList_Fix<TPosition, TValue>);
        ofs_ += sizeof(TPosition) * capacity;
        return (LinkedList_Fix<TPosition, TValue>*)(((char*)this) + ofs_);
    }

public:
    inline int                                      GetCapacity()
    {
        return this->m_capacity;
    }
    inline int                                      GetCount()
    {
        return this->m_count;
    }
    inline bool                                     Push(const TValue& item)
    {
        return this->Enqueue(item);
    }
    inline bool                                     Enqueue(const TValue& item)
    {
        int count = this->m_count;
        if (count >= this->m_capacity)
        {
            return false;
        }
        LinkedListNode_Fix<TPosition, TValue>* node = NULL;
        if (this->m_freestack <= this->m_frees)
        {
            node = &this->m_nodes[count];
        }
        else
        {
            node = this->m_linkedlist->GetNode(*--this->m_freestack); // �ӻ���ջ��ȡ���ڵ�
        }
        node->Value = item;
        this->m_linkedlist->AddLast(node);
        return count != ++this->m_count;
    }
    inline bool                                     TryDequeue(const TValue& item)
    {
        TValue* p = this->Dequeue();
        if (p == NULL)
        {
            return false;
        }
        else
        {
            const_cast<TValue&>(item) = *p;
        }
        return true;
    }
    inline bool                                     TryPop(const TValue& item)
    {
        TValue* p = this->Pop();
        if (p == NULL)
        {
            return false;
        }
        else
        {
            const_cast<TValue&>(item) = *p;
        }
        return true;
    }
    inline TValue*                                  Pop()
    {
        return this->PopOrDequeue(true);
    }
    inline TValue*                                  Dequeue()
    {
        return this->PopOrDequeue(false);
    }
    inline bool                                     TryPeek(const TValue& item)
    {
        int count = this->m_count;
        if (count <= 0)
        {
            return false;
        }
        LinkedListNode_Fix<TPosition, TValue>* node = this->m_linkedlist->Last();
        if (node == NULL)
        {
            return false;
        }
        const_cast<TValue&>(item) = node->Value;
        return true;
    }
    inline void                                     Clear()
    {
        memset(this->m_linkedlist, 0, sizeof(*this->m_linkedlist));
        this->m_count = 0;
        this->m_freestack = this->m_frees; // ���û���ջ��ָ��
    }
    inline int                                      CopyTo(std::set<TValue>& set)
    {
        LinkedList_Fix<TPosition, TValue>* linkedlist = this->m_linkedlist;
        int count = 0;
        if (linkedlist == NULL)
        {
            return count;
        }
        LinkedListNode_Fix<TPosition, TValue>* node = linkedlist->First();
        while (node != NULL && count++ < this->m_count)
        {
            set.insert(node->Value);
            node = node->GetNextNode(linkedlist);
        }
        return count;
    }

private:
    inline TValue*                                  PopOrDequeue(bool stack_mode)
    {
        int count = this->m_count;
        if (count <= 0)
        {
            return NULL;
        }
        LinkedListNode_Fix<TPosition, TValue>* node = NULL;
        if (stack_mode)
        {
            node = this->m_linkedlist->Last();
        }
        else
        {
            node = this->m_linkedlist->First();
        }
        if (!this->m_linkedlist->Remove(node))
        {
            return NULL;
        }
        *this->m_freestack++ = this->m_linkedlist->GetPosition(node); // ������ٵĻ���ջ��
        this->m_count--;
        return (TValue*)&reinterpret_cast<char&>(node->Value);
    }

public:
    inline bool                                     Initialize(int capacity)
    {
        if (capacity <= 0)
        {
            return false;
        }
        memset(this, 0, sizeof(BucketList_Fix<TPosition, TValue>));

        this->m_capacity = capacity;
        this->m_count = 0;

        this->m_linkedlist = this->GetLinkedListAddress(capacity);
        this->m_linkedlist->Initialize(capacity);

        this->m_frees = this->GetFreeNodeAddress(capacity);
        this->m_nodes = this->GetDataNodeAddress(capacity);
        this->m_freestack = this->m_frees;

        return true;
    }
    inline BucketList_Fix<TPosition, TValue>*       Attach(bool fast_mode = false)
    {
        return this->Attach(this, fast_mode);
    }
    template<typename TBucketList>
    inline BucketList_Fix<TPosition, TValue>*       Attach(TBucketList* data, bool fast_mode = false)
    {
        if (data == NULL)
        {
            return NULL;
        }
        BucketList_Fix<TPosition, TValue>* self = (BucketList_Fix<TPosition, TValue>*)data;
        self->m_linkedlist = self->GetLinkedListAddress(self->m_capacity); // �������������
        self->m_nodes = self->GetDataNodeAddress(self->m_capacity); // �������ݽڵ������
        // self->m_freestack = self->GetFreeNodeAddress(self->m_capacity) + (self->m_frees - self->m_freestack); // ��������ջ������
        self->m_frees = self->GetFreeNodeAddress(self->m_capacity); // �����ͷŽڵ������
        self->m_freestack = self->GetFreeNodeAddress(self->m_capacity);

        if (self->m_frees > self->m_freestack)
        {
            self->m_freestack = self->m_frees;
        }

        if (fast_mode) // FASTģʽ���Ӳ��������������޸���������ֻҪ���ǳ�ʽ��ǿ���ж��ǲ�����������ϵġ�
        {
            return self;
        }

        TPosition position = self->m_linkedlist->GetPosition(self->m_linkedlist->First());
        LinkedListNode_Fix<TPosition, TValue>* node = self->m_linkedlist->GetNode(position);

        int maxcount = self->m_count > self->m_linkedlist->Count() ? // ����������
            self->m_linkedlist->Count() :
            self->m_count;

        int repcount = 0; // �޸���
        self->m_count = 0;
        self->m_linkedlist->Clear();

        std::vector<TValue> datas(maxcount);
        while (node != NULL && repcount < maxcount) // kill ɱ�����̿��������δ����ȫ������Ҫ�ؽ���������
        {
            LinkedListNode_Fix<TPosition, TValue>* next = node->GetNextNode(self->m_linkedlist);
            datas[repcount++] = node->Value; // copyһ�����ݵ�datas֮�С�
            node = next;
        }
        TValue* chunks = datas.data();
        for (int i = 0, len = (int)datas.size(); i < len; i++)
        {
            self->Enqueue(chunks[i]);
        }
        return self;
    }
};