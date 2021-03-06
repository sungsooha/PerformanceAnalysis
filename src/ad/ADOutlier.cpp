#include "chimbuko/ad/ADOutlier.hpp"
#include "chimbuko/param/sstd_param.hpp"
#include "chimbuko/message.hpp"
#include <mpi.h>
#include <nlohmann/json.hpp>

#ifdef _PERF_METRIC
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MilliSec;
typedef std::chrono::microseconds MicroSec;
#endif

using namespace chimbuko;

/* ---------------------------------------------------------------------------
 * Implementation of ADOutlier class
 * --------------------------------------------------------------------------- */
ADOutlier::ADOutlier() 
: m_use_ps(false), m_execDataMap(nullptr), m_param(nullptr)
{
#ifdef _USE_ZMQNET
    m_context = nullptr;
    m_socket = nullptr;
#endif
}

ADOutlier::~ADOutlier() {
    if (m_param) {
        delete m_param;
    }
}

void ADOutlier::connect_ps(int rank, int srank, std::string sname) {
    m_rank = rank;
    m_srank = srank;

#ifdef _USE_MPINET
    int rs;
    char port[MPI_MAX_PORT_NAME];

    rs = MPI_Lookup_name(sname.c_str(), MPI_INFO_NULL, port);
    if (rs != MPI_SUCCESS) return;

    rs = MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &m_comm);
    if (rs != MPI_SUCCESS) return;

    // test connection
    Message msg;
    msg.set_info(m_rank, m_srank, MessageType::REQ_ECHO, MessageKind::DEFAULT);
    msg.set_msg("Hello!");

    MPINet::send(m_comm, msg.data(), m_srank, MessageType::REQ_ECHO, msg.count());

    MPI_Status status;
    int count;
    MPI_Probe(m_srank, MessageType::REP_ECHO, m_comm, &status);
    MPI_Get_count(&status, MPI_BYTE, &count);

    msg.clear();
    msg.set_msg(
        MPINet::recv(m_comm, status.MPI_SOURCE, status.MPI_TAG, count), true
    );

    if (msg.data_buffer().compare("Hello!>I am MPINET!") != 0)
    {
        std::cerr << "Connect error to parameter server (MPINET)!\n";
        exit(1);
    }
    m_use_ps = true;
    //std::cout << "rank: " << m_rank << ", " << msg.data_buffer() << std::endl;
#else
    m_context = zmq_ctx_new();
    m_socket = zmq_socket(m_context, ZMQ_REQ);
    zmq_connect(m_socket, sname.c_str());

    // test connection
    Message msg;
    std::string strmsg;

    msg.set_info(rank, srank, MessageType::REQ_ECHO, MessageKind::DEFAULT);
    msg.set_msg("Hello!");

    ZMQNet::send(m_socket, msg.data());

    msg.clear();
    ZMQNet::recv(m_socket, strmsg);
    msg.set_msg(strmsg, true);

    if (msg.buf().compare("\"Hello!I am ZMQNET!\"") != 0)
    {
        std::cerr << "Connect error to parameter server (ZMQNET)!\n";
        exit(1);
    } 
    m_use_ps = true;      
#endif
    //MPI_Barrier(MPI_COMM_WORLD);
}

void ADOutlier::disconnect_ps() {
    if (!m_use_ps) return;

    MPI_Barrier(MPI_COMM_WORLD);
    if (m_rank == 0)
    {
#ifdef _USE_MPINET
        Message msg;
        msg.set_info(m_rank, m_srank, MessageType::REQ_QUIT, MessageKind::CMD);
        msg.set_msg(MessageCmd::QUIT);
        MPINet::send(m_comm, msg.data(), m_srank, MessageType::REQ_QUIT, msg.count());
#else
        zmq_send(m_socket, nullptr, 0, 0);
#endif
    }
    MPI_Barrier(MPI_COMM_WORLD);

#ifdef _USE_MPINET
    MPI_Comm_disconnect(&m_comm);
#else
    zmq_close(m_socket);
    zmq_ctx_term(m_context);
#endif
    m_use_ps = false;
}

/* ---------------------------------------------------------------------------
 * Implementation of ADOutlierSSTD class
 * --------------------------------------------------------------------------- */
ADOutlierSSTD::ADOutlierSSTD() : ADOutlier(), m_sigma(6.0) {
    m_param = new SstdParam();
}

ADOutlierSSTD::~ADOutlierSSTD() {
}

std::pair<size_t,size_t> ADOutlierSSTD::sync_param(ParamInterface* param)
{
    SstdParam& g = *(SstdParam*)m_param;
    SstdParam& l = *(SstdParam*)param;

    if (!m_use_ps) {
        g.update(l);
        return std::make_pair(0, 0);
    }
    else {
        Message msg;
        std::string strmsg;
        size_t sent_sz, recv_sz;

        msg.set_info(m_rank, m_srank, MessageType::REQ_ADD, MessageKind::SSTD);
        msg.set_msg(l.serialize(), false);
        sent_sz = msg.size();
#ifdef _USE_MPINET
        MPINet::send(m_comm, msg.data(), m_srank, MessageType::REQ_ADD, msg.count());

        MPI_Status status;
        int count;
        MPI_Probe(m_srank, MessageType::REP_ADD, m_comm, &status);
        MPI_Get_count(&status, MPI_BYTE, &count);

        msg.clear();
        strmsg = MPINet::recv(m_comm, status.MPI_SOURCE, status.MPI_TAG, count);
#else
        ZMQNet::send(m_socket, msg.data());

        msg.clear();
        ZMQNet::recv(m_socket, strmsg);   
#endif
        msg.set_msg(strmsg , true);
        recv_sz = msg.size();
        g.assign(msg.buf());
        return std::make_pair(sent_sz, recv_sz);
    }
}

std::pair<size_t, size_t> ADOutlier::update_local_statistics(std::string l_stats, int step)
{
    if (!m_use_ps)
        return std::make_pair(0, 0);

    Message msg;
    std::string strmsg;
    size_t sent_sz, recv_sz;

    msg.set_info(m_rank, 0, MessageType::REQ_ADD, MessageKind::ANOMALY_STATS, step);
    msg.set_msg(l_stats);
    sent_sz = msg.size();
#ifdef _USE_MPINET
    throw "Not implemented yet.";
#else
    ZMQNet::send(m_socket, msg.data());

    msg.clear();
    ZMQNet::recv(m_socket, strmsg);
#endif
    // do post-processing, if necessary
    recv_sz = strmsg.size();

    return std::make_pair(sent_sz, recv_sz);
}

unsigned long ADOutlierSSTD::run(int step) {
    if (m_execDataMap == nullptr) return 0;

    SstdParam param;
    std::unordered_map<unsigned long, std::string> l_func;
    std::unordered_map<unsigned long, RunStats> l_inclusive;
    std::unordered_map<unsigned long, RunStats> l_exclusive;

    for (auto it : *m_execDataMap) {        
        for (auto itt : it.second) {
            param[it.first].push(static_cast<double>(itt->get_runtime()));
            l_func[it.first] = itt->get_funcname();
            l_inclusive[it.first].push(static_cast<double>(itt->get_inclusive()));
            l_exclusive[it.first].push(static_cast<double>(itt->get_exclusive()));
        }
    }

    // update temp runstats (parameter server)
#ifdef _PERF_METRIC
    Clock::time_point t0, t1;
    std::pair<size_t, size_t> msgsz;
    MicroSec usec;

    t0 = Clock::now();
    msgsz = sync_param(&param);
    t1 = Clock::now();
    
    usec = std::chrono::duration_cast<MicroSec>(t1 - t0);

    m_perf.add("param_update", (double)usec.count());
    m_perf.add("param_sent", (double)msgsz.first / 1000000.0); // MB
    m_perf.add("param_recv", (double)msgsz.second / 1000000.0); // MB
#else
    sync_param(&param);
#endif

    // run anomaly detection algorithm
    unsigned long n_outliers = 0;
    long min_ts = 0, max_ts = 0;

    // func id --> (name, # anomaly, inclusive run stats, exclusive run stats)
    nlohmann::json g_info;
    g_info["func"] = nlohmann::json::array();
    for (auto it : *m_execDataMap) {
        const unsigned long func_id = it.first;
        const unsigned long n = compute_outliers(func_id, it.second, min_ts, max_ts);
        n_outliers += n;

        nlohmann::json obj;
        obj["id"] = func_id;
        obj["name"] = l_func[func_id];
        obj["n_anomaly"] = n;
        obj["inclusive"] = l_inclusive[func_id].get_json_state();
        obj["exclusive"] = l_exclusive[func_id].get_json_state();
        g_info["func"].push_back(obj);
    }
    g_info["anomaly"] = AnomalyData(0, m_rank, step, min_ts, max_ts, n_outliers).get_json();

    // update # anomaly
#ifdef _PERF_METRIC
    t0 = Clock::now();
    msgsz = update_local_statistics(g_info.dump(), step);
    t1 = Clock::now();

    usec = std::chrono::duration_cast<MicroSec>(t1 - t0);

    m_perf.add("stream_update", (double)usec.count());
    m_perf.add("stream_sent", (double)msgsz.first / 1000000.0); // MB
    m_perf.add("stream_recv", (double)msgsz.second / 1000000.0); // MB    
#else
    update_local_statistics(g_info.dump(), step);
#endif

    return n_outliers;
}

unsigned long ADOutlierSSTD::compute_outliers(
    const unsigned long func_id, 
    std::vector<CallListIterator_t>& data,
    long& min_ts, long& max_ts) 
{
    SstdParam& param = *(SstdParam*)m_param;
    if (param[func_id].count() < 2) return 0;
    unsigned long n_outliers = 0;

    const double mean = param[func_id].mean();
    const double std = param[func_id].stddev();

    const double thr_hi = mean + m_sigma * std;
    const double thr_lo = mean - m_sigma * std;

    for (auto itt : data) {
        const double runtime = static_cast<double>(itt->get_runtime());
        if (min_ts == 0 || min_ts > itt->get_entry())
            min_ts = itt->get_entry();
        if (max_ts == 0 || max_ts < itt->get_exit())
            max_ts = itt->get_exit();

        int label = (thr_lo > runtime || thr_hi < runtime) ? -1: 1;
        if (label == -1) {
            n_outliers += 1;
            itt->set_label(label);
        }
    }

    return n_outliers;
}
