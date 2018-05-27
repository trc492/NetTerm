#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="DList.h" />
///
/// <summary>
///     This module contains the definition and implementation of the
///     DList class.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (04-Jan-2007)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#ifdef MOD_ID
    #undef MOD_ID
#endif
#define MOD_ID                  MOD_DLIST

class DList
{
private:
    //
    // Private data.
    //
    DWORD            m_dwcEntries;
    LIST_ENTRY       m_listHead;
    CRITICAL_SECTION m_CritSect;

public:
    /**
     *  Constructor of the class object.
     */
    DList(
        VOID
        ): m_dwcEntries(0)
    {
        TLevel(INIT);
        TEnter();

        m_listHead.Flink = m_listHead.Blink = &m_listHead;
        __try
        {
            InitializeCriticalSection(&m_CritSect);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            TErr(("Failed to initialize critical section (err=%d).",
                  GetExceptionCode()));
        }

        TExit();
        return;
    }   //DList

    /**
     *  Desctructor of the class object.
     */
    ~DList(
        VOID
        )
    {
        TLevel(INIT);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        if (m_listHead.Flink != &m_listHead)
        {
            //
            // The caller did not deallocate the list before destroying it.
            // The DList object is only responsible for the head entry.
            // So all we can do is to detach the list from the head node
            // before deallocating the DList object.  This is to make sure
            // nobody is referencing deallocated data later on.
            //
            TWarn(("DList is not empty (Entries=%d).", m_dwcEntries));
            if (m_listHead.Flink != NULL)
            {
                m_listHead.Flink->Blink = m_listHead.Blink;
            }
            if (m_listHead.Blink != NULL)
            {
                m_listHead.Blink->Flink = m_listHead.Flink;
            }
            m_listHead.Flink = m_listHead.Blink = &m_listHead;
        }
        m_dwcEntries = 0;
        LeaveCriticalSection(&m_CritSect);
        DeleteCriticalSection(&m_CritSect);

        TExit();
        return;
    }   //~DList

    /**
     *  This function checks if the DList is empty.
     *
     *  @return Success: Returns TRUE.
     *  @return Failure: Returns FALSE.
     */
    BOOL
    IsDListEmpty(
        VOID
        )
    {
        BOOL rc;

        TLevel(API);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        rc = (m_listHead.Flink == &m_listHead);
        TAssert(rc && (m_dwcEntries == 0) || !rc && (m_dwcEntries > 0));
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("= %d (Entries=%d)", rc, m_dwcEntries));
        return rc;
    }   //IsDListEmpty

    /**
     *  This function removes the given entry from the link list.
     *
     *  @param entry Points to the entry to be removed.
     */
    VOID
    RemoveEntryDList(
        __in PLIST_ENTRY entry
        )
    {
        TLevel(API);
        TEnterMsg(("entry=%p", entry));

        EnterCriticalSection(&m_CritSect);
        if ((entry != NULL) && (entry != &m_listHead) && (m_dwcEntries > 0))
        {
            if ((entry->Flink != NULL) && (entry->Blink != NULL))
            {
                entry->Flink->Blink = entry->Blink;
                entry->Blink->Flink = entry->Flink;
                entry->Flink = entry->Blink = NULL;
                m_dwcEntries--;
            }
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg((" (Entries=%d)", m_dwcEntries));
        return;
    }   //RemoveEntryDList

    /**
     *  This function removes the first entry from the link list.
     *
     *  @return Success; Returns the pointer to the entry removed.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    RemoveHeadDList(
        VOID
        )
    {
        PLIST_ENTRY entry = NULL;

        TLevel(API);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        if ((m_listHead.Flink != &m_listHead) && (m_dwcEntries > 0))
        {
            entry = m_listHead.Flink;
            if ((entry->Flink != NULL) && (entry->Blink != NULL))
            {
                entry->Flink->Blink = entry->Blink;
                entry->Blink->Flink = entry->Flink;
                entry->Flink = entry->Blink = NULL;
                m_dwcEntries--;
            }
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p (Entries=%d,HeadFlink=%p,HeadBlink=%p)",
                  entry, m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return entry;
    }   //RemoveHeadDList

    /**
     *  This function removes the last entry from the link list.
     *
     *  @return Success; Returns the pointer to the entry removed.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    RemoveTailDList(
        VOID
        )
    {
        PLIST_ENTRY entry = NULL;

        TLevel(API);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        if ((m_listHead.Blink != &m_listHead) && (m_dwcEntries > 0))
        {
            entry = m_listHead.Blink;
            if ((entry->Flink != NULL) && (entry->Blink != NULL))
            {
                entry->Flink->Blink = entry->Blink;
                entry->Blink->Flink = entry->Flink;
                entry->Flink = entry->Blink = NULL;
                m_dwcEntries--;
            }
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p (Entries=%d,HeadFlink=%p,HeadBlink=%p)",
                  entry, m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return entry;
    }   //RemoveTailDList

    /**
     *  This function gets the first entry from the link list.
     *
     *  @return Success; Returns the pointer to the first entry.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    GetHeadDList(
        VOID
        )
    {
        PLIST_ENTRY entry = NULL;

        TLevel(API);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        if ((m_listHead.Flink != &m_listHead) && (m_dwcEntries > 0))
        {
            entry = m_listHead.Flink;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p (Entries=%d,HeadFlink=%p,HeadBlink=%p)",
                  entry, m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return entry;
    }   //GetHeadDList

    /**
     *  This function gets the last entry from the link list.
     *
     *  @return Success; Returns the pointer to the last entry.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    GetTailDList(
        VOID
        )
    {
        PLIST_ENTRY entry = NULL;

        TLevel(API);
        TEnter();

        EnterCriticalSection(&m_CritSect);
        if ((m_listHead.Blink != &m_listHead) && (m_dwcEntries > 0))
        {
            entry = m_listHead.Blink;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p (Entries=%d,HeadFlink=%p,HeadBlink=%p)",
                  entry, m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return entry;
    }   //GetTailDList

    /**
     *  This function gets the previous entry of a given entry from the
     *  link list.
     *
     *  @param entry Points to an entry to retrieve its backward link.
     *
     *  @return Success; Returns the pointer to the previous entry.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    GetPrevDList(
        __in PLIST_ENTRY entry
        )
    {
        PLIST_ENTRY prevEntry = NULL;

        TLevel(API);
        TEnterMsg(("entry=%p", entry));

        EnterCriticalSection(&m_CritSect);
        if ((entry != NULL) &&
            (entry != &m_listHead) &&
            (entry->Blink != &m_listHead))
        {
            prevEntry = entry->Blink;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p", prevEntry));
        return prevEntry;
    }   //GetPrevDList

    /**
     *  This function gets the next entry of a given entry from the link list.
     *
     *  @param entry Points to an entry to retrieve its forward link.
     *
     *  @return Success; Returns the pointer to the next entry.
     *  @return Failure: Returns NULL.
     */
    PLIST_ENTRY
    GetNextDList(
        __in PLIST_ENTRY entry
        )
    {
        PLIST_ENTRY nextEntry = NULL;

        TLevel(API);
        TEnterMsg(("entry=%p", entry));

        EnterCriticalSection(&m_CritSect);
        if ((entry != NULL) &&
            (entry != &m_listHead) &&
            (entry->Flink != &m_listHead))
        {
            nextEntry = entry->Flink;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("=%p", nextEntry));
        return nextEntry;
    }   //GetNextDList

    /**
     *  This function inserts the given entry to the head of the link list.
     *
     *  @param entry Points to an entry to be inserted.
     */
    VOID
    InsertHeadDList(
        __in PLIST_ENTRY entry
        )
    {
        TLevel(API);
        TEnterMsg(("entry=%p", entry));

        EnterCriticalSection(&m_CritSect);
        if (entry != NULL)
        {
            entry->Flink = m_listHead.Flink;
            entry->Blink = &m_listHead;
            m_listHead.Flink->Blink = entry;
            m_listHead.Flink = entry;
            m_dwcEntries++;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("! (Entries=%d,HeadFlink=%p,HeadBlink=%p",
                  m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return;
    }   //InsertHeadDList

    /**
     *  This function inserts the given entry to the tail of the link list.
     *
     *  @param entry Points to an entry to be inserted.
     */
    VOID
    InsertTailDList(
        __in PLIST_ENTRY entry
        )
    {
        TLevel(API);
        TEnterMsg(("entry=%p", entry));

        EnterCriticalSection(&m_CritSect);
        if (entry != NULL)
        {
            entry->Flink = &m_listHead;
            entry->Blink = m_listHead.Blink;
            m_listHead.Blink->Flink = entry;
            m_listHead.Blink = entry;
            m_dwcEntries++;
        }
        LeaveCriticalSection(&m_CritSect);

        TExitMsg(("! (Entries=%d,HeadFlink=%p,HeadBlink=%p",
                  m_dwcEntries, m_listHead.Flink, m_listHead.Blink));
        return;
    }   //InsertTailDList

    /**
     *  This function returns the number of entries in the link list.
     *
     *  @return Returns the number of entries in the link list.
     */
    DWORD
    QueryEntriesDList(
        VOID
        )
    {
        TLevel(API);
        TEnter();
        TExitMsg(("=%d", m_dwcEntries));
        return m_dwcEntries;
    }   //QueryEntriesDList

    /**
     *  This function acquires the critical section for accessing the
     *  link list.
     */
    //_Acquires_lock_(this->m_CritSect)
    VOID
    EnterCritSect(
        VOID
        )
    {
        TLevel(API);
        TEnter();
        EnterCriticalSection(&m_CritSect);
        TExit();
        return;
    }   //EnterCritSect

    /**
     *  This function releases the critical section for accessing the
     *  link list.
     */
    //_Releases_lock_(this->m_CritSect)
    VOID
    LeaveCritSect(
        VOID
        )
    {
        TLevel(API);
        TEnter();
        LeaveCriticalSection(&m_CritSect);
        TExit();
        return;
    }   //LeaveCritSect

};  //class DList

