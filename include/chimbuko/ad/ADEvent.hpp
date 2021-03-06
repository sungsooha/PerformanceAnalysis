#pragma once
#include "chimbuko/ad/ADDefine.hpp"
#include "chimbuko/ad/ExecData.hpp"
#include <string>
#include <vector>
#include <list>
#include <stack>
#include <unordered_map>

namespace chimbuko {

/**
 * @brief a stack of CommData_t
 * 
 */
typedef std::stack<CommData_t> CommStack_t;

/**
 * @brief hash map of CommStack_t in thread level
 * 
 */
typedef std::unordered_map<unsigned long, CommStack_t>      CommStackMap_t_t;

/**
 * @brief hash map of CommStackMap_t_t in rank level
 * 
 */
typedef std::unordered_map<unsigned long, CommStackMap_t_t> CommStackMap_r_t;

/**
 * @brief hash map of CommStackMap_r_t in application level
 * 
 */
typedef std::unordered_map<unsigned long, CommStackMap_r_t> CommStackMap_p_t;

/**
 * @brief list of function calls (ExecData_t) in entry time order
 * 
 */
typedef std::list<ExecData_t> CallList_t;

/**
 * @brief iterator of CallList_t
 * 
 */
typedef CallList_t::iterator  CallListIterator_t;

/**
 * @brief hash map of CallList_t in thread level
 * 
 */
typedef std::unordered_map<unsigned long, CallList_t>      CallListMap_t_t;

/**
 * @brief hash map of CallListMap_t_t in rank level
 * 
 */
typedef std::unordered_map<unsigned long, CallListMap_t_t> CallListMap_r_t;

/**
 * @brief hash map of CallListMap_r_t in application level
 * 
 */
typedef std::unordered_map<unsigned long, CallListMap_r_t> CallListMap_p_t;

/**
 * @brief function call stack
 * 
 */
typedef std::stack<CallListIterator_t> CallStack_t;

/**
 * @brief hash map of CallStack_t in thread level
 * 
 */
typedef std::unordered_map<unsigned long, CallStack_t>      CallStackMap_t_t;

/**
 * @brief hash map of CallStackMap_t_t in rank level
 * 
 */
typedef std::unordered_map<unsigned long, CallStackMap_t_t> CallStackMap_r_t;

/**
 * @brief hash map of CallStackMap_r_t in application level
 * 
 */
typedef std::unordered_map<unsigned long, CallStackMap_r_t> CallStackMap_p_t;

/**
 * @brief hash map of a collection of ExecData_t per function
 * 
 * key is function id and value is a vector of CallListIterator_t (i.e. ExecData_t)
 * 
 */
typedef std::unordered_map<unsigned long, std::vector<CallListIterator_t>> ExecDataMap_t;

/**
 * @brief Event manager
 * 
 */
class ADEvent {

public:
    /**
     * @brief Construct a new ADEvent object
     * 
     * @param verbose true to print out detailed information (useful for debug)
     */
    ADEvent(bool verbose=false);
    /**
     * @brief Destroy the ADEvent object
     * 
     */
    ~ADEvent();

    /**
     * @brief copy a pointer that is externally defined even type object 
     * 
     * @param m event type object (hash map)
     */
    void linkEventType(const std::unordered_map<int, std::string>* m) { m_eventType = m; }

    /**
     * @brief copy a pointer that is externally defined function map object
     * 
     * @param m function map object
     */
    void linkFuncMap(const std::unordered_map<int, std::string>* m) { m_funcMap = m; }

    /**
     * @brief Get the Func Map object
     * 
     * @return const std::unordered_map<int, std::string>* pointer to function map object
     */
    const std::unordered_map<int, std::string>* getFuncMap() const { return m_funcMap; }

    /**
     * @brief Get the Event Type object
     * 
     * @return const std::unordered_map<int, std::string>* pointer to event type object
     */
    const std::unordered_map<int, std::string>* getEventType() const { return m_eventType; }

    /**
     * @brief Get the Exec Data Map object
     * 
     * @return const ExecDataMap_t* pointer to ExecDataMap_t object
     */
    const ExecDataMap_t* getExecDataMap() const { return &m_execDataMap; }

    /**
     * @brief Get the Call List Map object
     * 
     * @return const CallListMap_p_t* pointer to CallListMap_p_t object
     */
    const CallListMap_p_t * getCallListMap() const { return &m_callList; }
    /**
     * @brief Get the Call List Map object
     * 
     * @return CallListMap_p_t& pointer to CallListMap_p_t object
     */
    CallListMap_p_t& getCallListMap() { return m_callList; }

    /**
     * @brief clear 
     * 
     */
    void clear();

    /**
     * @brief add an event
     * 
     * @param event function or communication event
     * @return EventError event error code
     */
    EventError addEvent(const Event_t& event);
    /**
     * @brief add a function event
     * 
     * @param event function event
     * @return EventError event error code
     */
    EventError addFunc(const Event_t& event);
    /**
     * @brief add a communication event
     * 
     * @param event communication event
     * @return EventError event error code
     */
    EventError addComm(const Event_t& event);

    /**
     * @brief trim out all function calls that are completed (i.e. a pair of ENTRY and EXIT events are observed)
     * 
     * @return CallListMap_p_t* trimed function calls
     */
    CallListMap_p_t* trimCallList();
    /**
     * @brief show current call stack tree status
     * 
     * @param verbose true to see all details
     */
    void show_status(bool verbose=false) const;

private:
    /**
     * @brief Create a communication stack for the given pid, rid, tid
     * 
     * @param pid application index
     * @param rid rank index
     * @param tid thread index
     */
    void createCommStack(unsigned long pid, unsigned long rid, unsigned long tid);
    /**
     * @brief Create a Call stack for the given pid, rid, tid
     * 
     * @param pid application index
     * @param rid rank index
     * @param tid thread index
     */
    void createCallStack(unsigned long pid, unsigned long rid, unsigned long tid);
    /**
     * @brief Create a Call List for the given pid, rid, tid
     * 
     * @param pid application index
     * @param rid rank index
     * @param tid thread index
     */
    void createCallList(unsigned long pid, unsigned long rid, unsigned long tid);

private:
    /**
     * @brief pointer to function map
     * 
     */
    const std::unordered_map<int, std::string> *m_funcMap;
    /**
     * @brief pointer to event type
     * 
     */
    const std::unordered_map<int, std::string> *m_eventType;

    /**
     * @brief communication stack
     * 
     */
    CommStackMap_p_t  m_commStack;
    /**
     * @brief function call stack
     * 
     */
    CallStackMap_p_t  m_callStack;
    /**
     * @brief function call list
     * 
     */
    CallListMap_p_t   m_callList;
    /**
     * @brief execution data map
     * 
     */
    ExecDataMap_t     m_execDataMap;

    /**
     * @brief verbose
     * 
     */
    bool m_verbose;
};

} // end of AD namespace