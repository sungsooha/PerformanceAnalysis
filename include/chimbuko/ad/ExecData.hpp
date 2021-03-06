#pragma once
#include "chimbuko/ad/ADDefine.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace chimbuko {

/**
 * @brief class to provide easy access to raw performance event vector
 * 
 */
class Event_t {
public:
    /**
     * @brief Construct a new Event_t object
     * 
     * @param data pointer to raw performance event vector
     * @param t event type
     * @param idx event index
     * @param id event (string) id
     */
    Event_t(const unsigned long * data, EventDataType t, size_t idx, std::string id="event_id") 
        : m_data(data), m_t(t), m_id(id), m_idx(idx) {}
    /**
     * @brief Destroy the Event_t object
     * 
     */
    ~Event_t() {}

    /**
     * @brief check if the raw data pointer is valid 
     */
    bool valid() const { 
        return m_data != nullptr; 
    }
    /**
     * @brief return event id
     */
    std::string id() const { return m_id; }
    /**
     * @brief return event index
     */
    size_t idx() const { return m_idx; }
    /**
     * @brief return program id
     */
    unsigned long pid() const { return m_data[IDX_P]; }
    /**
     * @brief return rank id
     */
    unsigned long rid() const { return m_data[IDX_R]; }
    /**
     * @brief return thread id
     */
    unsigned long tid() const { return m_data[IDX_T]; }
    /**
     * @brief return event id
     */
    unsigned long eid() const { return m_data[IDX_E]; }   
    /**
     * @brief return timestamp of this event
     */
    unsigned long ts() const;
    
    /**
     * @brief return event type
     */
    EventDataType type() const { return m_t; }
    /**
     * @brief return string event type
     */
    std::string strtype() const;

    /**
     * @brief return function (timer) id
     */
    unsigned long fid() const;

    /**
     * @brief return communication tag id
     */
    unsigned long tag() const;
    /**
     * @brief return communication partner id
     */
    unsigned long partner() const;
    /**
     * @brief return communication data size (in bytes)
     */
    unsigned long bytes() const;

    /**
     * @brief compare two events
     */
    friend bool operator<(const Event_t& lhs, const Event_t& rhs);
    /**
     * @brief compare two events
     */
    friend bool operator>(const Event_t& lhs, const Event_t& rhs);

    /**
     * @brief Get the json object of this event object
     */
    nlohmann::json get_json() const;

private:
    const unsigned long * m_data;   /**< pointer to raw performance trace data vector */
    EventDataType m_t;              /**< event type */
    std::string m_id;               /**< event id */
    size_t m_idx;                   /**< event index */
};

/**
 * @brief compare two events
 */
bool operator<(const Event_t& lhs, const Event_t& rhs);
/**
 * @brief compare two events
 */
bool operator>(const Event_t& lhs, const Event_t& rhs);


/**
 * @brief wrapper for communication event
 * 
 */
class CommData_t {
public:
    /**
     * @brief Construct a new CommData_t object
     * 
     */
    CommData_t();
    /**
     * @brief Construct a new CommData_t object
     * 
     * @param ev constant reference to a Event_t object
     * @param commType communication type (e.g. SEND/RECV)
     */
    CommData_t(const Event_t& ev, std::string commType);
    /**
     * @brief Destroy the CommData_t object
     * 
     */
    ~CommData_t() {};

    /**
     * @brief return communication type
     */
    std::string type() const { return m_commType; }
    /**
     * @brief return timestamp
     */
    unsigned long ts() const { return m_ts; }
    /**
     * @brief return source process id of this communication event
     */
    unsigned long src() const { return m_src; }
    /**
     * @brief return target (or destination) process id of this communication event
     */
    unsigned long tar() const { return m_tar; }

    /**
     * @brief Set the execution key id (i.e. where this communication event occurs)
     * 
     * @param key execution id
     */
    void set_exec_key(std::string key) { m_execkey = key; }

    /**
     * @brief compare two communication data
     * 
     * @param other 
     * @return true if other is same
     * @return false if other is different
     */
    bool is_same(const CommData_t& other) const;
    /**
     * @brief Get the json object of this communication data
     */
    nlohmann::json get_json() const;

private:
    std::string m_commType;             /**< communication type */
    unsigned long 
        m_pid,                          /**< program id */
        m_rid,                          /**< rank id */
        m_tid;                          /**< thread id */
    unsigned long 
        m_src,                          /**< source process id */
        m_tar;                          /**< target process id */
    unsigned long 
        m_bytes,                        /**< communication data size in bytes */
        m_tag;                          /**< communication tag */
    unsigned long m_ts;                 /**< communication timestamp */
    std::string m_execkey;              /**< execution key (or id) where this communication event occurs */
};

/**
 * @brief A pair of function (timer) events, ENTRY and EXIT.
 * 
 */
class ExecData_t {

public:
    /**
     * @brief Construct a new ExecData_t object
     * 
     */
    ExecData_t();
    /**
     * @brief Construct a new ExecData_t object
     * 
     * @param ev constant reference to a Event_t object
     */
    ExecData_t(const Event_t& ev);
    /**
     * @brief Destroy the ExecData_t object
     * 
     */
    ~ExecData_t();

    /**
     * @brief Get the id of this execution data
     */
    std::string get_id() const { return m_id; }
    /**
     * @brief Get the function name of this execution data
     */
    std::string get_funcname() const { return m_funcname; }
    /**
     * @brief Get the program id of this execution data
     */
    unsigned long get_pid() const { return m_pid; }
    /**
     * @brief Get the thread id of this execution data
     */
    unsigned long get_tid() const { return m_tid; }
    /**
     * @brief Get the rank id of this execution data
     */
    unsigned long get_rid() const { return m_rid; }
    /**
     * @brief Get the function id of this execution data
     */
    unsigned long get_fid() const { return m_fid; }
    /**
     * @brief Get the entry time of this execution data
     */
    long get_entry() const { return m_entry; }
    /**
     * @brief Get the exit time of this execution data
     */
    long get_exit() const { return m_exit; }
    /**
     * @brief Get the (inclusive) running time of this execution data
     */
    long get_runtime() const { return m_runtime; }
    /**
     * @brief Get the (inclusive) running time of this execution data
     */
    long get_inclusive() const { return m_runtime; }
    /**
     * @brief Get the exclusive running ime of this execution data
     */
    long get_exclusive() const { return m_exclusive; }
    /**
     * @brief Get the label of this execution data
     * 
     * @return int 1 of normal and -1 os anomaly
     */
    int get_label() const { return m_label; }
    /**
     * @brief Get the parent function id of this execution data
     */
    std::string get_parent() const { return m_parent; }
    /**
     * @brief Get a list of communication data occured in this execution data
     */
    std::vector<CommData_t>& get_messages() { return m_messages; }

    /**
     * @brief Get the number of communication events
     */
    unsigned long get_n_message() const { return m_n_messages; }
    /**
     * @brief Get the number of childrent functions
     */
    unsigned long get_n_children() const { return m_n_children; }

    /**
     * @brief Set the label 
     * 
     * @param label 1 for normal, -1 for anomaly
     */
    void set_label(int label) { m_label = label; }
    /**
     * @brief Set the parent function of this execution
     * 
     * @param parent the parent execution id
     */
    void set_parent(std::string parent) { m_parent = parent; }
    /**
     * @brief Set the function name of this execution
     * 
     * @param funcname function name
     */
    void set_funcname(std::string funcname) { m_funcname = funcname; }

    /**
     * @brief update exit event of this execution
     * 
     * @param ev exit event
     * @return true no errors
     * @return false incorrect exit event
     */
    bool update_exit(const Event_t& ev);
    /**
     * @brief update exclusive running time
     * 
     * @param t running time of a childrent function
     */
    void update_exclusive(long t) { m_exclusive -= t; }
    /**
     * @brief increase the number of childrent function by 1
     * 
     */
    void inc_n_children() { m_n_children++; }
    /**
     * @brief add communication data
     * 
     * @param comm communication event occured in this execution
     * @return true no errors
     * @return false invalid communication event
     */
    bool add_message(CommData_t& comm);

    /**
     * @brief compare with other execution
     * 
     * @param other other execution data
     * @return true if they are same
     * @return false if they are different
     */
    bool is_same(const ExecData_t& other) const;

    /**
     * @brief Get the json object of this execution data
     * 
     * @param with_message if true, including all message (communication) information
     * @return nlohmann::json json object
     */
    nlohmann::json get_json(bool with_message=false) const;
    
private:
    std::string m_id;                    /**< execution id */                           
    std::string m_funcname;              /**< function name */
    unsigned long 
        m_pid,                           /**< program id */
        m_tid,                           /**< thread id */
        m_rid,                           /**< rank id */
        m_fid;                           /**< function id */
    long 
        m_entry,                         /**< entry time */
        m_exit,                          /**< exit time */
        m_runtime,                       /**< inclusive running time */
        m_exclusive;                     /**< exclusive running time */
    int m_label;                         /**< 1 for normal, -1 for abnormal execution */
    std::string m_parent;                /**< parent execution */
    unsigned long m_n_children;          /**< the number of childrent executions */
    unsigned long m_n_messages;          /**< the number of messages */
    std::vector<CommData_t> m_messages;  /**< a vector of all messages */
};

} // end of AD namespace
