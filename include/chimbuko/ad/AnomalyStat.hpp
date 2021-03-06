#pragma once
#include <mutex>
#include <string>
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include "chimbuko/util/RunStats.hpp"

namespace chimbuko {

/**
 * @brief 
 * 
 */
class AnomalyData {
public:
    AnomalyData();

    AnomalyData(
        unsigned long app, unsigned long rank, unsigned step,
        unsigned long min_ts, unsigned long max_ts, unsigned long n_anomalies,
        std::string stat_id="");

    AnomalyData(const std::string& s);

    ~AnomalyData()
    {
    }

    void set(
        unsigned long app, unsigned long rank, unsigned step,
        unsigned long min_ts, unsigned long max_ts, unsigned long n_anomalies,
        std::string stat_id="");

    unsigned long get_app() const { return m_app; }
    unsigned long get_rank() const { return m_rank; }
    unsigned long get_step() const { return m_step; }
    unsigned long get_min_ts() const { return m_min_timestamp; }
    unsigned long get_max_ts() const { return m_max_timestamp; }
    unsigned long get_n_anomalies() const { return m_n_anomalies; }
    std::string get_stat_id() const { return m_stat_id; }

    friend bool operator==(const AnomalyData& a, const AnomalyData& b);
    friend bool operator!=(const AnomalyData& a, const AnomalyData& b);

    nlohmann::json get_json() const;

private:
    unsigned long m_app;
    unsigned long m_rank;
    unsigned long m_step;
    unsigned long m_min_timestamp;
    unsigned long m_max_timestamp;
    unsigned long m_n_anomalies;
    std::string   m_stat_id;
};

bool operator==(const AnomalyData& a, const AnomalyData& b);
bool operator!=(const AnomalyData& a, const AnomalyData& b);

class AnomalyStat {
public:
    AnomalyStat(bool do_accumulate=false);
    ~AnomalyStat();

    void add(AnomalyData& d, bool bStore = true);
    void add(const std::string& str, bool bStore = true);

    /**
     * @brief Get copy of the current statistics and the pointer 
     *        to data list.
     * 
     * WARN: Once this function is called, the pointer to the 
     * current data list is returned and new (empty) data list is
     * allocated. So, it is callee's responsibility to free the 
     * allocated memory.
     * 
     * @return std::pair<RunStats, std::list<std::string>*> 
     */
    std::pair<RunStats, std::list<std::string>*> get();
    RunStats get_stats();

    /**
     * @brief Get the pointer to the data list
     * 
     * WARN: As it returns the pointer to the data list,
     * new data can be added to the list in other threads. 
     * Also, it shouldn't be freed by the callee.
     * 
     * @return std::list<std::string>* 
     */
    std::list<std::string>* get_data();
    size_t get_n_data();

private:
    std::mutex               m_mutex;
    RunStats                 m_stats;
    std::list<std::string> * m_data;
};

} // end of chimbuko namespace
