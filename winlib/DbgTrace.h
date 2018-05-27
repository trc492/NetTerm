#if 0
/// Copyright (c) Titan Robotics Club. All rights reserved.
///
/// <module name="DbgTrace.h" />
///
/// <summary>
///     This module contains the definitions and implementation of the
///     DbgTrace class.
/// </summary>
///
/// <remarks>
///     Author: Michael Tsang (25-Apr-2012)
///     Environment: Windows application.
/// </remarks>
#endif

#pragma once

#define MOD_MAIN                0x00000001
#define TGenModId(n)            (MOD_MAIN << (n))

//
// Constants.
//
#define NONE                    0
#define INIT                    1
#define API                     2
#define CALLBK                  3
#define EVENT                   4
#define FUNC                    5
#define TASK                    6
#define UTIL                    7
#define HIFREQ                  8

#define FATAL                   0
#define ERR                     1
#define WARN                    2
#define INFO                    3
#define VERBOSE                 4

//
// Macros.
//
#define TPrintf                 printf

#if defined(_ENABLE_FUNCTRACE) || defined(_ENABLE_MSGTRACE)
  #ifndef _ENABLE_TRACING
    #define _ENABLE_TRACING
  #endif
#endif

//
// Trace macros.
//
#ifdef _ENABLE_TRACING
    #define TEnable(b)          g_Trace.m_fTraceEnabled = (b)
    #define TraceInit(m,l,e)    g_Trace.Initialize(m, l, e)
    #define TLevel(l)           int _traceLevel = l
#else
    #define TEnable(b)
    #define TraceInit(m,l,e)
    #define TLevel(l)
#endif

#ifdef _ENABLE_FUNCTRACE
    #define TModEnterMsg(m,p)   if (g_Trace.m_fTraceEnabled && \
                                    ((g_Trace.m_traceModules & (m)) != 0) && \
                                    (_traceLevel <= g_Trace.m_traceLevel)) \
                                { \
                                    g_Trace.FuncPrefix(__FUNCTION__, \
                                                       true, \
                                                       false); \
                                    TPrintf p; \
                                    TPrintf(")\n"); \
                                }
    #define TModEnter(m)        if (g_Trace.m_fTraceEnabled && \
                                    ((g_Trace.m_traceModules & (m)) != 0) && \
                                    (_traceLevel <= g_Trace.m_traceLevel)) \
                                { \
                                    g_Trace.FuncPrefix(__FUNCTION__, \
                                                       true, \
                                                       true); \
                                }
    #define TModExitMsg(m,p)    if (g_Trace.m_fTraceEnabled && \
                                    ((g_Trace.m_traceModules & (m)) != 0) && \
                                    (_traceLevel <= g_Trace.m_traceLevel)) \
                                { \
                                    g_Trace.FuncPrefix(__FUNCTION__, \
                                                       false, \
                                                       false); \
                                    TPrintf p; \
                                    TPrintf("\n"); \
                                }
    #define TModExit(m)         if (g_Trace.m_fTraceEnabled && \
                                    ((g_Trace.m_traceModules & (m)) != 0) && \
                                    (_traceLevel <= g_Trace.m_traceLevel)) \
                                { \
                                    g_Trace.FuncPrefix(__FUNCTION__, \
                                                       false, \
                                                       true); \
                                }
    #define TEnterMsg(p)        TModEnterMsg(MOD_ID, p)
    #define TEnter()            TModEnter(MOD_ID)
    #define TExitMsg(p)         TModExitMsg(MOD_ID, p)
    #define TExit()             TModExit(MOD_ID)
#else
    #define TEnterMsg(p)
    #define TEnter()
    #define TExitMsg(p)
    #define TExit()
#endif

#ifdef _ENABLE_MSGTRACE
    #define TModMsg(m,e,p)      if (((g_Trace.m_traceModules & (m)) != 0) && \
                                    ((e) <= g_Trace.m_msgLevel)) \
                                { \
                                    g_Trace.MsgPrefix(__FUNCTION__, \
                                                      e); \
                                    TPrintf p; \
                                    TPrintf("\n"); \
                                }
    #define TMsg(e,p)           if ((e) <= g_Trace.m_msgLevel) \
                                { \
                                    g_Trace.MsgPrefix(__FUNCTION__, \
                                                      e); \
                                    TPrintf p; \
                                    TPrintf("\n"); \
                                }
    #define TFatal(p)           TMsg(FATAL, p)
    #define TErr(p)             TMsg(ERR, p)
    #define TWarn(p)            TMsg(WARN, p)
    #define TInfo(p)            TModMsg(MOD_ID, INFO, p)
    #define TVerbose(p)         TModMsg(MOD_ID, VERBOSE, p)
    #define TAssert(e)          if (!(e)) \
                                { \
                                    TPrintf("%s_Assert: Assertion at line %d in file %s\n", \
                                            __FUNCTION__, __LINE__, __FILE__); \
                                }
#else
    #define TMsg(e,p)
    #define TFatal(p)
    #define TErr(p)
    #define TWarn(p)
    #define TInfo(p)
    #define TVerbose(p)
    #define TAssert(e)
#endif

/**
 * This class implements the debug tracing object. It provides two facilities.
 * One allows the functions to trace the enter and exit conditions of the call
 * by dumping the calling parameters of function entry and the return value
 * of function exit. The other one allows the function to print out different
 * level of messages such as fatal message, error message, warning message,
 * info message and verbose message etc.
 */
class DbgTrace
{
public:
    BOOL    m_fTraceEnabled;
    DWORD   m_traceModules;
    int     m_traceLevel;
    int     m_msgLevel;

private:
    #define MAX_NUM_THREADS 16
    DWORD   m_threadIds[MAX_NUM_THREADS];
    int     m_indentLevels[MAX_NUM_THREADS];
    int     m_currThreadIdx;

public:
    /**
     * Constructor for the DbgTrace object.
     */
    DbgTrace(
        void
        ): m_fTraceEnabled(FALSE)
         , m_traceModules(MOD_MAIN)
         , m_traceLevel(NONE)
         , m_msgLevel(WARN)
    {
        for (int i = 0; i < MAX_NUM_THREADS; i++)
        {
            m_threadIds[i] = 0;
            m_indentLevels[i] = 0;
        }
        m_currThreadIdx = -1;
    }   //DbgTrace

    /**
     * Destructor for the DbgTrace object.
     */
    virtual
    ~DbgTrace(
        void
        )
    {
    }   //~DbgTrace

    /**
     * This function initializes the tracing module with the specified
     * module IDs and trace levels.
     *
     * @param traceModules Bit mask specifying which modules to enable tracing
     *        with. Each module is assigned a bit ID in the bit mask.
     * @param traceLevel Specifies the function trace level at or below which
     *        function tracing is enabled.
     * @param msgLevel Specifies the message trace level at or below which
     *        message tracing is enabled.
     */
    void
    Initialize(
        UINT32 traceModules,
        UINT32 traceLevel,
        UINT32 msgLevel
        )
    {
        m_traceModules = traceModules;
        m_traceLevel = traceLevel;
        m_msgLevel = msgLevel;
        m_fTraceEnabled = TRUE;
    }   //Initialize

    /**
     * This method generates the function trace prefix string. The prefix
     * contains the indentation, the module name and the function name.
     *
     * @param pszFunc Specifies the name of the function.
     * @param fEnter Specifies whether we are entering or exiting the
     *        function.
     * @param fNewLine Specifies whether we will print a new line.
     */
    void
    FuncPrefix(
        const char *pszFunc,
        bool        fEnter,
        bool        fNewLine
        )
    {
        DWORD currThreadId = GetCurrentThreadId();
        int nIndent = 0;
        int idx;

        if (m_currThreadIdx == -1)
        {
            //
            // This is the first time we are called, so the thread table is
            // empty. Initialize and use the first entry.
            //
            m_currThreadIdx = 0;
            m_threadIds[0] = currThreadId;
            m_indentLevels[0] = 0;
        }
        else if ((m_currThreadIdx == -2) ||
                 (currThreadId != m_threadIds[m_currThreadIdx]))
        {
            int emptyEntryIdx = -1;
            //
            // Either we have switched thread or the table was full, let's
            // rescan the table to find the thread or an empty slot for it.
            //
            for (idx = 0; idx < MAX_NUM_THREADS; idx++)
            {
                if (currThreadId == m_threadIds[idx])
                {
                    //
                    // Found it, make it the current thread.
                    //
                    m_currThreadIdx = idx;
                    break;
                }
                else if ((emptyEntryIdx == -1) && (m_threadIds[idx] == 0))
                {
                    //
                    // It's the first empty slot we found, remember it so
                    // if we can't find the thread at the end, we will store
                    // it here.
                    //
                    emptyEntryIdx = idx;
                }
            }

            if (idx == MAX_NUM_THREADS)
            {
                //
                // We didn't find the thread in the table.
                //
                if (emptyEntryIdx != -1)
                {
                    //
                    // Make a new entry using the first empty slot we found.
                    //
                    m_currThreadIdx = emptyEntryIdx;
                    m_threadIds[m_currThreadIdx] = currThreadId;
                    m_indentLevels[m_currThreadIdx] = 0;
                }
                else
                {
                    //
                    // The table is full.
                    //
                    m_currThreadIdx = -2;
                }
            }
        }

        if (m_currThreadIdx >= 0)
        {
            nIndent = fEnter? ++m_indentLevels[m_currThreadIdx]:
                              m_indentLevels[m_currThreadIdx];
        }

        TPrintf("%08x:", currThreadId);

        for (int i = 0; i < nIndent; i++)
        {
            TPrintf("| ");
        }

        TPrintf("%s", pszFunc);

        if (fEnter)
        {
            TPrintf("%s", fNewLine? "()\n": "(");
        }
        else
        {
            TPrintf("%s", fNewLine? "!\n": "");
            if (m_currThreadIdx >= 0)
            {
                m_indentLevels[m_currThreadIdx]--;
                if (m_indentLevels[m_currThreadIdx] == 0)
                {
                    m_threadIds[m_currThreadIdx] = 0;
                }
            }
        }
    }   //FuncPrefix

    /**
     * This method generates the message trace prefix string. The prefix
     * contains the module and function names as well as message level info
     * in which the message is printed.
     *
     * @param pszFunc Specifies the name of the function.
     * @param msgLevel Specifies message level.
     */
    void
    MsgPrefix(
        const char *pszFunc,
        UINT32      msgLevel
        )
    {
        char *pszPrefix = "_Unk: ";

        switch (msgLevel)
        {
        case FATAL:
            pszPrefix = "_Fatal: ";
            break;

        case ERR:
            pszPrefix = "_Err: ";
            break;

        case WARN:
            pszPrefix = "_Warn: ";
            break;

        case INFO:
            pszPrefix = "_Info: ";
            break;

        case VERBOSE:
            pszPrefix = "_Verbose: ";
            break;
        }

        TPrintf("%s%s", pszFunc, pszPrefix);
    }   //MsgPrefix
};	//class DbgTrace

#ifdef _MAIN_FILE
    DbgTrace g_Trace;
#else
    extern DbgTrace g_Trace;
#endif

