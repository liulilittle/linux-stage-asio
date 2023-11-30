#pragma once

#include <stdint.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32)

#include <set>
#include <map>
#include <hash_set>
#include <hash_map>
#include <malloc.h>

#include "linkedlist.h"
#else

#include "util/Util.h"
#include "util/memory/MemDefine.h"
#include "util/collections/linkedlist.h"
#endif

#include <assert.h>

template<typename TValue>
class ConcurrentSet
{
private:
    typedef LinkedListNode<TValue>              EntryNode;
#if defined(WIN32) || defined(_WIN32)

    typedef std::map<TValue, EntryNode*>        EntryTable;                 // stdext::hash_map
    typedef std::set<EntryNode*>                AvailableEntrySet;          // stdext::hash_set
#else

    typedef HASHMAPDEFINE(TValue, EntryNode*)   EntryTable;
    typedef HASHSETDEFINE(EntryNode*)           AvailableEntrySet;
#endif

private:
    LinkedList<TValue>                          m_oLinkedList;
    EntryTable                                  m_oEntryTable;
    AvailableEntrySet                           m_oAvailableNodeSet;

public:
    class Enumerator
    {
        friend class                            ConcurrentSet<TValue>;

    public:
        Enumerator()
            : m_pSet(NULL)
            , m_bRst(true)
            , m_pCurrentEntry(NULL)
        {

        }
        Enumerator(const Enumerator& enumerator)
            : m_pSet(enumerator.m_pSet)
            , m_bRst(enumerator.m_bRst)
            , m_pCurrentEntry(enumerator.m_pCurrentEntry)
        {

        }
        Enumerator(ConcurrentSet* pSet)
            : m_pSet(pSet)
            , m_bRst(true)
            , m_pCurrentEntry(NULL)
        {

        }
        ~Enumerator()
        {
            //Reset();
            //m_pSet = NULL;
        }

    public:
        inline void                             Reset()
        {
            m_bRst = true;
            m_pCurrentEntry = NULL;
        }
        inline ConcurrentSet*                   GetOwner()
        {
            return m_pSet;
        }
        inline TValue*                          GetCurrent()
        {
            if (NULL == m_pSet)
            {
                return NULL;
            }

            if (NULL == m_pCurrentEntry)
            {
                return NULL;
            }

            if (!m_pSet->IsAvailableEntry(m_pCurrentEntry))
            {
                m_pCurrentEntry = NULL;
                return NULL;
            }

            return (TValue*)&reinterpret_cast<const char&>(m_pCurrentEntry->Value);
        }
        inline bool                             MoveNext()
        {
            if (NULL == m_pSet)
            {
                return false;
            }

            if (m_bRst)
            {
                m_bRst = false;
                m_pCurrentEntry = m_pSet->GetFirst();
                return NULL != m_pCurrentEntry;
            }

            if (NULL == m_pCurrentEntry)
            {
                return false;
            }

            if (!m_pSet->IsAvailableEntry(m_pCurrentEntry))
            {
                m_pCurrentEntry = NULL;
            }
            else
            {
                m_pCurrentEntry = m_pCurrentEntry->Next;
            }

            return NULL != m_pCurrentEntry;
        }
        inline bool                             IsAvailable()
        {
            EntryNode* node = this->m_pCurrentEntry;
            if (NULL == node)
            {
                return false;
            }

            ConcurrentSet<TValue>* pSet = this->m_pSet;
            if (NULL == pSet)
            {
                return false;
            }

            return pSet->IsAvailableEntry(node);
        }

    private:
        inline Enumerator&                      MoveToBegin(int m)
        {
            m_pCurrentEntry = NULL;

            if (m != 1)
            {
                if (NULL != m_pSet)
                {
                    m_pCurrentEntry = m_pSet->GetFirst();
                }
            }

            m_bRst = NULL == m_pCurrentEntry;
            return *this;
        }
        inline Enumerator&                      MoveToEnd(int m)
        {
            m_pCurrentEntry = NULL;
            if (m == 1)
            {
                m_pCurrentEntry = m_pSet->GetLast();
            }

            m_bRst = NULL == m_pCurrentEntry;
            return *this;
        }
        inline Enumerator&                      MoveTo(EntryNode* entry)
        {
            if (NULL == entry)
            {
                return MoveToEnd(0);
            }

            m_pCurrentEntry = NULL;
            if (NULL != m_pSet)
            {
                m_pCurrentEntry = entry;
            }

            m_bRst = NULL == m_pCurrentEntry;
            return *this;
        }

    public:
        inline const Enumerator&                operator ++ ()
        {
            MoveNext();
            return *this;
        }
        inline const Enumerator&                operator ++ (int)
        {
            return ++(*this);
        }
        inline const Enumerator&                operator -- ()
        {
            do
            {
                if (NULL == m_pSet)
                {
                    break;
                }

                if (m_bRst)
                {
                    m_bRst = false;
                    m_pCurrentEntry = m_pSet->GetLast();
                    break;
                }

                if (NULL == m_pCurrentEntry)
                {
                    break;
                }

                if (!m_pSet->IsAvailableEntry(m_pCurrentEntry))
                {
                    m_pCurrentEntry = NULL;
                }
                else
                {
                    m_pCurrentEntry = m_pCurrentEntry->Previous;
                }
            } while (false);

            return *this;
        }
        inline const Enumerator&                operator -- (int)
        {
            return --(*this);
        }
        inline const TValue&                    operator * ()
        {
            return *GetCurrent();
        }
        inline bool                             operator == (const Enumerator& right)
        {
            return right.m_pCurrentEntry == this->m_pCurrentEntry;
        }
        inline bool                             operator != (const Enumerator& right)
        {
            return right.m_pCurrentEntry != this->m_pCurrentEntry;
        }

    private:
        ConcurrentSet*                          m_pSet;
        volatile bool                           m_bRst;
        EntryNode*                              m_pCurrentEntry;
    };

public:
    typedef Enumerator                          iterator;

public:
    ConcurrentSet()
    {

    }
    ~ConcurrentSet()
    {
        Clear();
    }

public:
    inline int                                  GetCount()
    {
        return m_oLinkedList.Count();
    }
    inline bool                                 ContansKey(const TValue& value)
    {
        return m_oEntryTable.find(value) != m_oEntryTable.end();
    }
    inline bool                                 Insert(const TValue& value)
    {
        typename EntryTable::iterator itEntry = m_oEntryTable.find(value);
        if (itEntry != m_oEntryTable.end())
        {
            return false;
        }

        EntryNode* pEntryNode = AllocEntry();
        if (NULL == pEntryNode)
        {
            return false;
        }

        pEntryNode->Value = value;

        std::pair<typename EntryTable::iterator, bool> kvRet;
        kvRet = m_oEntryTable.insert(std::make_pair(value, pEntryNode));

        if (!kvRet.second)
        {
            DeleteEntry(pEntryNode);
            return false;
        }

        m_oLinkedList.AddLast(pEntryNode);
        m_oAvailableNodeSet.insert(pEntryNode);

        return true;
    }
    inline bool                                 Remove(const TValue& value)
    {
        typename EntryTable::iterator itEntry = m_oEntryTable.find(value);
        if (itEntry == m_oEntryTable.end())
        {
            return false;
        }
        else
        {
            EntryNode* pEntryNode = itEntry->second;

            m_oLinkedList.Remove(pEntryNode);
            m_oAvailableNodeSet.erase(pEntryNode);
            m_oEntryTable.erase(itEntry);

            DeleteEntry(pEntryNode);
        }
        return true;
    }
    inline void                                 Clear()
    {
        EntryNode* node = m_oLinkedList.First();
        while (NULL != node)
        {
            EntryNode* current = node;
            node = node->Next;

            DeleteEntry(current);
        }

        m_oEntryTable.clear();
        m_oAvailableNodeSet.clear();
        memset(&m_oLinkedList, 0, sizeof(m_oLinkedList));
    }

public:
    inline EntryNode*                           GetFirst()
    {
        int nCnt = m_oLinkedList.Count();
        if (0 >= nCnt)
        {
            return NULL;
        }

        return m_oLinkedList.First();
    }
    inline EntryNode*                           GetLast()
    {
        int nCnt = m_oLinkedList.Count();
        if (0 >= nCnt)
        {
            return NULL;
        }

        return m_oLinkedList.Last();
    }
    inline Enumerator                           GetEnumerator()
    {
        return Enumerator(this);
    }
    inline bool                                 IsAvailable(const EntryNode* entry)
    {
        return IsAvailableEntry(const_cast<EntryNode*>(entry));
    }

public:
    // STL
    inline iterator                             begin()
    {
        return Enumerator(this).MoveToBegin(0);
    }
    inline iterator                             end()
    {
        return Enumerator(this).MoveToEnd(0);
    }
    inline iterator                             rbegin()
    {
        return Enumerator(this).MoveToBegin(1);
    }
    inline iterator                             rend()
    {
        return Enumerator(this).MoveToEnd(1);
    }
    inline void                                 clear()
    {
        Clear();
    }
    inline iterator                             find(const TValue& value)
    {
        typename EntryTable::iterator itEntryNode = m_oEntryTable.find(value);
        if (itEntryNode == m_oEntryTable.end())
        {
            return this->end();
        }

        EntryNode* ptrEntryNode = itEntryNode->second;
        if (NULL == ptrEntryNode)
        {
            return this->end();
        }

        return Enumerator(this).MoveTo(ptrEntryNode);
    }
    inline size_t                               size()
    {
        int len = GetCount();
        return len <= 0 ? 0 : len & 0x7fffffff;
    }
    inline bool                                 empty()
    {
        return GetCount() <= 0;
    }
    inline bool                                 erase(const iterator& i)
    {
        TValue* p = i->GetCurrent();
        if (NULL == p)
        {
            return false;
        }

        return erase(*p);
    }
    inline bool                                 erase(const TValue& value)
    {
        return Remove(value);
    }
    inline bool                                 insert(const TValue& value)
    {
        return Insert(value);
    }

private:
    inline EntryNode*                           AllocEntry()
    {
        EntryNode* node = NULL;
#if defined(WIN32) || defined(_WIN32)
        node = (EntryNode*)malloc(sizeof(EntryNode));
        if (NULL == node)
        {
            return NULL;
        }

        memset(node, 0, sizeof(*node));
#else
        node = (EntryNode*)MemAlloc(sizeof(EntryNode));
        if (NULL == node)
        {
            return NULL;
        }
#endif
        return node;
    }
    inline void                                 DeleteEntry(EntryNode* entry)
    {
        if (NULL != entry)
        {
#if defined(WIN32) || defined(_WIN32)
            free(entry);
#else
            MemFree2(entry, sizeof(*entry));
#endif
        }
    }
    inline bool                                 IsAvailableEntry(EntryNode* entry)
    {
        if (NULL == entry || NULL == this)
        {
            return false;
        }

        if (m_oLinkedList.Count() <= 0)
        {
            return false;
        }

        if (NULL == m_oLinkedList.First())
        {
            memset(&m_oLinkedList, 0, sizeof(m_oLinkedList));
            return false;
        }

        return m_oAvailableNodeSet.find(entry) != m_oAvailableNodeSet.end();
    }
};