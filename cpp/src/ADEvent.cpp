#include "ADEvent.hpp"
#include <iostream>

ADEvent::ADEvent() : m_funcMap(nullptr), m_eventType(nullptr) {

}

ADEvent::~ADEvent() {
    clear();
}

void ADEvent::clear() {
    // for (auto it_p = m_callStack.begin(); it_p != m_callStack.end(); it_p++)
    //     for (auto it_r = it_p->second.begin(); it_r != it_p->second.end(); it_r++)
    //         for (auto it_t = it_r->second.begin(); it_t != it_r->second.end(); it_t++)
    //         {
    //             ADEvent::CallStack_t& cs = it_t->second;
    //             while (!cs.empty())
    //             {
    //                 ExecData_t* exec = cs.top();
    //                 cs.pop();
    //                 delete exec;
    //             }
    //         }

    // for (auto it_p = m_callList.begin(); it_p != m_callList.end(); it_p++)
    //     for (auto it_r = it_p->second.begin(); it_r != it_p->second.end(); it_r++)
    //         for (auto it_t = it_r->second.begin(); it_t != it_r->second.end(); it_t++)
    //         {
    //             ADEvent::CallList_t& cl = it_t->second;
    //             for (auto it=cl.begin(); it != cl.end(); it++)
    //             {
    //                 ExecData_t* exec = *it;
    //                 cl.erase(it);
    //                 delete exec;
    //             }
    //         }
}

void ADEvent::createCallStack(unsigned long pid, unsigned long rid, unsigned long tid) {
    if (m_callStack.count(pid) == 0)
        m_callStack[pid] = ADEvent::CallStackMap_r_t();

    if (m_callStack[pid].count(rid) == 0)
        m_callStack[pid][rid] = ADEvent::CallStackMap_t_t();

    if (m_callStack[pid][rid].count(tid) == 0)
        m_callStack[pid][rid][tid] = ADEvent::CallStack_t();
}

void ADEvent::createCallList(unsigned long pid, unsigned long rid, unsigned long tid) {
    if (m_callList.count(pid) == 0)
        m_callList[pid] = ADEvent::CallListMap_r_t();
    
    if (m_callList[pid].count(rid) == 0)
        m_callList[pid][rid] = ADEvent::CallListMap_t_t();

    if (m_callList[pid][rid].count(tid) == 0)
        m_callList[pid][rid][tid] = ADEvent::CallList_t();
}

EventError ADEvent::addFunc(FuncEvent_t event, std::string event_id) {
    if (m_eventType == nullptr)
        return EventError::UnknownEvent;

    std::string eventType;
    int eid = static_cast<int>(event.eid());
    if (m_eventType->count(eid) == 0)
        return EventError::UnknownEvent;

    this->createCallStack(event.pid(), event.rid(), event.tid());
    this->createCallList(event.pid(), event.rid(), event.tid());

    eventType = m_eventType->find(eid)->second;
    
    if (eventType.compare("ENTRY") == 0)
    {
        ADEvent::CallList_t& cl = m_callList[event.pid()][event.rid()][event.tid()];
        cl.push_front(ExecData_t(event_id, event));

        ADEvent::CallListIterator_t it = cl.begin();
        CallStack_t& cs = m_callStack[event.pid()][event.rid()][event.tid()];
        if (cs.size()) {
            it->set_parent(cs.top()->get_id());
            cs.top()->add_child(it->get_id());
        }
        it->set_funcname(m_funcMap->find(event.fid())->second);
        cs.push(it);

        return EventError::OK;
    }
    else if (eventType.compare("EXIT") == 0)
    {
        CallStack_t& cs = m_callStack[event.pid()][event.rid()][event.tid()];
        if (cs.size() == 0) {
            std::cout << "Empty call stack!" << std::endl;
            return EventError::CallStackViolation;
        }

        ADEvent::CallListIterator_t it = cs.top();
        cs.pop();

        if (!it->update_exit(event)) {
            std::cout << "Invalid exit!" << std::endl;
            return EventError::CallStackViolation;
        }

        // todo: update depth (is this needed?)
        m_execDataMap[event.fid()].push_back(it);

        // std::cout << *it << std::endl;

        return EventError::OK;
    }

    return EventError::UnknownEvent;
}