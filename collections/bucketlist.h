#pragma once

/************************************************************************/
/* ���¿ɼ���֪��֪��                                                   */
/************************************************************************/
#include <malloc.h>
#include <set>
#include "linkedlist.h"

template<typename T>
class BucketList // Ͱ
{
private:
    LinkedList<T>*              m_linkedlist;
    struct
    {
        LinkedListNode<T>*      node_rbp;               // ָ�����Ľڵ�
        LinkedListNode<T>**     free_rbp;               // ָ���ͷŵĽڵ�
        LinkedListNode<T>**     free_rsp_stack;         // ָ���ͷŽڵ�Ļ���ջ
    } m_chunks;
    struct
    {
        LinkedList<T>*          linkedlist;             // ָ���������BucketList�ĵ�ַ            
        LinkedListNode<T>*      node_fst;
        LinkedListNode<T>**     free_rsp_stack;
    } m_rvas;
    BucketList<T>*              m_self_va;              // ָ������ľ��Ե�ַ
    bool                        m_disposed;
    bool                        m_release;
    volatile int                m_count;
    volatile int                m_capacity;

    // Ͱ���ڴ������ͬһ�������ڴ������ʱ���������òű����֡�
    template<typename E>
    inline E* VA2RVA(const E* VA)
    {
        if (VA == NULL)
        {
            return NULL;
        }
        long long x = (long long)this->m_self_va;
        long long y = (long long)VA;
        return (E*)(y - x);
    }
    template<typename E>
    inline E* RVA2VA(const E* RVA)
    {
        if (RVA == NULL) // RVA�ڵ�ǰ�ĳ����б��޶���Զ������NULL
        {
            return NULL;
        }
        long long x = (long long)RVA;
        long long y = (long long)this->m_self_va;
        return (E*)(x + y);
    }
    inline void Update()
    {
        this->m_rvas.node_fst = this->VA2RVA(this->m_linkedlist->First());
        this->m_rvas.free_rsp_stack = this->VA2RVA(this->m_chunks.free_rsp_stack);
    }
    inline LinkedListNode<T>* GetDataNodeAddress(int capacity) // ��ȡ���ݽڵ�ĵ�ַ
    {
        int size_ = sizeof(BucketList<T>);
        size_ += sizeof(LinkedList<T>);
        return (LinkedListNode<T>*)(((char*)this) + size_);
    }
    inline LinkedListNode<T>** GetFreeNodeAddress(int capacity)
    {
        int size_ = sizeof(BucketList<T>);
        size_ += sizeof(LinkedList<T>);
        size_ += sizeof(LinkedListNode<T>) * capacity;
        return (LinkedListNode<T>**)(((char*)this) + size_);
    }
    inline LinkedList<T>* GetLinkedListAddress()
    {
        int size_ = sizeof(BucketList<T>);
        return (LinkedList<T>*)(((char*)this) + size_);
    }
public:
    BucketList()
    {
        memset(this, 0, sizeof(BucketList<T>));
        this->m_self_va = this;

    }
protected:
    BucketList(bool release, int capacity) // ������ù��죬��ǰ��������������棬�ÿ�ѡĬ��ֵ���档
    {
        memset(this, 0, sizeof(BucketList<T>));

        this->m_capacity = capacity;
        this->m_self_va = this;
        this->m_release = release;

        this->m_linkedlist = new (this->GetLinkedListAddress()) LinkedList<T>();

        this->m_chunks.node_rbp = this->GetDataNodeAddress(this->m_capacity);
        this->m_chunks.free_rbp = this->GetFreeNodeAddress(this->m_capacity);
        this->m_chunks.free_rsp_stack = this->m_chunks.free_rbp;

        this->m_rvas.node_fst = NULL;
        this->m_rvas.free_rsp_stack = NULL;
        this->m_rvas.linkedlist = VA2RVA(this->m_linkedlist);
    }
public:
    ~BucketList()
    {
        this->Dispose();
    }
    bool IsDisposed()
    {
        return this->m_disposed;
    }
    void Dispose()
    {
        if (!this->m_disposed)
        {
            bool release = this->m_release;
            memset(this, 0, sizeof(BucketList<T>));
            this->m_disposed = true;
            this->m_release = release;
        }
    }
    inline int GetCapacity()
    {
        return this->m_capacity;
    }
    inline int GetCount()
    {
        return this->m_count;
    }
    inline bool Push(const T& item)
    {
        return this->Enqueue(item);
    }
    inline bool Enqueue(const T& item)
    {
        if (this->m_disposed)
        {
            return false;
        }
        int count = this->m_count;
        if (count >= this->m_capacity)
        {
            return false;
        }
        LinkedListNode<T>* node = NULL;
        if (this->m_chunks.free_rsp_stack <= this->m_chunks.free_rbp)
        {
            node = &this->m_chunks.node_rbp[count];
        }
        else
        {
            node = *--this->m_chunks.free_rsp_stack; // �ӻ���ջ��ȡ���ڵ�
        }
        node->Value = item;
        {
            this->m_linkedlist->AddLast(node);
            this->Update();
        }
        return count != ++this->m_count;
    }
    inline bool TryDequeue(const T& item)
    {
        T* p = this->Dequeue();
        if (p == NULL)
        {
            return false;
        }
        else
        {
            const_cast<T&>(item) = *p;
        }
        return true;
    }
    inline bool TryPop(const T& item)
    {
        T* p = this->Pop();
        if (p == NULL)
        {
            return false;
        }
        else
        {
            const_cast<T&>(item) = *p;
        }
        return true;
    }
    inline T* Pop()
    {
        return this->PopOrDequeue(true);
    }
    inline T* Dequeue()
    {
        return this->PopOrDequeue(false);
    }
    inline bool TryPeek(const T& item)
    {
        if (this->m_disposed)
        {
            return false;
        }
        int count = this->m_count;
        if (count <= 0)
        {
            return false;
        }
        LinkedListNode<T>* node = this->m_linkedlist->Last();
        if (node == NULL)
        {
            return false;
        }
        const_cast<T&>(item) = node->Value;
        return true;
    }
    inline void Clear()
    {
        if (!this->m_disposed)
        {
            memset(this->m_linkedlist, 0, sizeof(*this->m_linkedlist));
            this->m_count = 0;
            this->m_chunks.free_rsp_stack = this->m_chunks.free_rbp; // ���û���ջ��ָ��
        }
    }
    inline int SizeOf()
    {
        return BucketList<T>::SizeOf(this->GetCapacity());
    }
    inline int CopyTo(std::set<T>& set)
    {
        if (this->m_disposed)
        {
            return -1;
        }
        LinkedListNode<T>* node = this->m_linkedlist->First();
        int count = 0;
        while (node != NULL && count++ < this->m_count)
        {
            set.insert(node->Value);
            node = node->Next;
        }
        return count;
    }
private:
    inline T* PopOrDequeue(bool stack_mode)
    {
        if (this->m_disposed)
        {
            return NULL;
        }
        int count = this->m_count;
        if (count <= 0)
        {
            return NULL;
        }
        LinkedListNode<T>* node = NULL;
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
        *this->m_chunks.free_rsp_stack++ = node; // ������ٵĻ���ջ��
        {
            this->m_count--;
            this->Update();
        }
        return (T*)&reinterpret_cast<char&>(node->Value);
    }
public:
    static int SizeOf(int capacity)
    {
        int size_ = sizeof(BucketList<T>);
        size_ += sizeof(LinkedList<T>);
        size_ += sizeof(LinkedListNode<T>) * capacity;
        size_ += sizeof(LinkedListNode<T>*) * capacity;
        return size_;
    }
    static BucketList<T>* New(int capacity)
    {
        int size_ = SizeOf(capacity);
        char* buffer_ = NULL;
        if (size_ <= 0 || (buffer_ = (char*)malloc(size_)) == NULL)
        {
            return NULL;
        }
        return new (buffer_) BucketList<T>(true, capacity);
    }
    static void Delete(const BucketList<T>* bucket)
    {
        if (bucket != NULL)
        {
            BucketList<T>* self = const_cast<BucketList<T>*>(bucket);
            self->~BucketList();
            if (self->m_release)
            {
                free(self);
            }
        }
    }
    static BucketList<T>* PlacementNew(const void* address, int capacity)
    {
        if (address == NULL)
        {
            return NULL;
        }
        char* buffer_ = (char*)address;
        return new (buffer_) BucketList<T>(false, capacity);
    }
    static BucketList<T>* Attach(const void* address)
    {
        if (address == NULL)
        {
            return NULL;
        }
        BucketList<T>* self = (BucketList<T>*)address; // ͨ�������ڴ滹ԭ���ľ��Ե�ַ��������δ��ȡ���Ե���Ե�ַ�����˷���Ŀǰʵ�������ٶȱȽϿ죩
        if (self->IsDisposed()) // �������ͷŵ��������ֱ�ӷ��ء�
        {
            return self;
        }
        BucketList<T>* before_ptr = self->m_self_va;
        self->m_self_va = self; // ָ���ĵ�ַ��֮ǰ��ͬ
        self->m_linkedlist = self->RVA2VA(self->m_rvas.linkedlist); // ��ԭ����ľ��Ե�ַ

        if (self->m_count <= 0)
        {
            memset(self->m_linkedlist, 0, sizeof(*self->m_linkedlist));
            self->m_count = 0;

            self->m_chunks.node_rbp = self->GetDataNodeAddress(self->m_capacity);
            self->m_chunks.free_rbp = self->GetFreeNodeAddress(self->m_capacity);
            self->m_chunks.free_rsp_stack = self->m_chunks.free_rbp;

            self->Update();
            return self;
        }
        LinkedListNode<T>* node_fst = self->m_linkedlist->First();
        memset(self->m_linkedlist, 0, sizeof(*self->m_linkedlist));
        node_fst = self->RVA2VA(self->m_rvas.node_fst); // ָ������һ���ڵ�ĵ�ַ�����Եĵ�ַ��

        LinkedListNode<T>* node_ptr = node_fst;
        int node_count = 0; // int node_frva = (int)((char*)self->GetDataNodeAddress(self->m_capacity) - (char*)self);
        while (node_ptr != NULL && node_count++ < self->m_count) // �����������ӵĵ�ַ
        {
            LinkedListNode<T>* next_node = node_ptr->Next;
            if (next_node != NULL)
            {
                next_node = self->RVA2VA((LinkedListNode<T>*)((char*)next_node - (char*)before_ptr)); // (node_frva + ((char*)next_node - (char*)self->m_chunks.node_rbp))
            }
            self->m_linkedlist->AddLast(node_ptr);
            node_ptr = next_node;
        }

        self->m_chunks.node_rbp = self->GetDataNodeAddress(self->m_capacity);
        self->m_chunks.free_rbp = self->GetFreeNodeAddress(self->m_capacity);
        self->m_chunks.free_rsp_stack = self->RVA2VA(self->m_rvas.free_rsp_stack);

        if (self->m_chunks.free_rbp > self->m_chunks.free_rsp_stack)
        {
            self->m_chunks.free_rsp_stack = self->m_chunks.free_rbp;
        }
        for (LinkedListNode<T>** node_i = self->m_chunks.free_rbp; node_i < self->m_chunks.free_rsp_stack; node_i++)
        {
            *node_i = self->RVA2VA((LinkedListNode<T>*)((char*)*node_i - (char*)before_ptr)); // (node_frva + ((char*)*node_i - (char*)self->m_chunks.node_rbp))
        }
        return self;
    }
};