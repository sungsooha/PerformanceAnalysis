#!/bin/bash
# ADIOS mode: [SST, BPFile]
ADIOS_MODE=${1:-SST}
# true if BPFile is available (currently, it must be false for docker run)
HAS_BPFILE=${2:-false}

# anomaly detection parameters
# sigma value, [6]
AD_SIGMA=${3:-6}
# window size, [10]
AD_WINSZ=${4:-10}
# time interval (only for BP mode for simulation), [1000]
AD_INTERVAL=${5:-1000}

# nwchem
# data steps
DATA_STEPS=${6:-50000}

echo "============================"
echo "ADIOS: ${ADIOS_MODE}"
echo "HAS BPFile: ${HAS_BPFILE}"
echo "AD SIGMA: ${AD_SIGMA}"
echo "AD WINSZ: ${AD_WINSZ}"
echo "AD INTERVAL: ${AD_INTERVAL} msec"
echo "NWCHEM DATA STEPS: ${DATA_STEPS}"
echo "============================"
sleep 10

export NWCHEM_TOP=/Codar/nwchem-1
export NWCHEM_DAT=$NWCHEM_TOP/QA/tests/ethanol

export CHIMBUKO_ROOT=/opt/chimbuko
export CHIMBUKO_VIS_ROOT=/Downloads/ChimbukoVisualizationII

export TAU_ROOT=/opt/tau2/x86_64
export TAU_MAKEFILE=$TAU_ROOT/lib/Makefile.tau-papi-mpi-pthread-pdt-adios2
export TAU_PLUGINS_PATH=$TAU_ROOT/lib/shared-papi-mpi-pthread-pdt-adios2
export TAU_PLUGINS=libTAU-adios2-trace-plugin.so


mkdir -p test
cd test
rm -rf DB executions logs
mkdir -p logs
mkdir -p DB
mkdir -p BP
mkdir -p executions
WORK_DIR=`pwd`

BP_PREFIX=tau-metrics-nwchem
export TAU_ADIOS2_PERIODIC=1
export TAU_ADIOS2_PERIOD=1000000
export TAU_ADIOS2_SELECTION_FILE=$WORK_DIR/sos_filter.txt
export TAU_ADIOS2_ENGINE=$ADIOS_MODE
export TAU_ADIOS2_FILENAME=$WORK_DIR/BP/tau-metrics
#export TAU_VERBOSE=1

# visualization server
export SERVER_CONFIG="production"
export DATABASE_URL="sqlite:///${WORK_DIR}/DB/main.sqlite"
export ANOMALY_STATS_URL="sqlite:///${WORK_DIR}/DB/anomaly_stats.sqlite"
export ANOMALY_DATA_URL="sqlite:///${WORK_DIR}/DB/anomaly_data.sqlite"
export FUNC_STATS_URL="sqlite:///${WORK_DIR}/DB/func_stats.sqlite"
export EXECUTION_PATH=$WORK_DIR/executions

cp $NWCHEM_TOP/bin/LINUX64/nwchem .
cp $NWCHEM_DAT/ethanol_md.nw .
cp $NWCHEM_DAT/*.pdb .
cp $NWCHEM_DAT/ethanol_md.rst .
cp $NWCHEM_DAT/ethanol_md.out .
cp $CHIMBUKO_ROOT/scripts/sos_filter.txt .
cp -r $CHIMBUKO_ROOT/bin .
cp -r $CHIMBUKO_ROOT/lib .

sed -i 's/coord 0/coord 1/' ethanol_md.nw
sed -i 's/scoor 0/scoor 1/' ethanol_md.nw
sed -i 's/step 0.001/step 0.001/' ethanol_md.nw
sed -i '21s|set|#set|' ethanol_md.nw
sed -i '22s|#set|set|' ethanol_md.nw
sed -i "s|data 1000|data ${DATA_STEPS}| ethanol_md.nw

date
hostname
ls -l

NMPIS=5



# echo ""
# echo "=========================================="
# echo "Launch Chimbuko visualization server"
# echo "=========================================="
cd $CHIMBUKO_VIS_ROOT

echo "run redis ..."
webserver/run-redis.sh &
#>"${WORK_DIR}/logs/redis.log" &
#2>&1 &
sleep 10

echo "run celery ..."
# only for docker
export C_FORCE_ROOT=1
# python3 manager.py celery --loglevel=info --concurrency=10 -f "${WORK_DIR}/logs/celery.log" &
python3 manager.py celery --loglevel=info --concurrency=4 --logfile=${WORK_DIR}/logs/celery.log &
#    >"${WORK_DIR}/logs/celery.log" 2>&1 &
sleep 10

echo "create database @ ${DATABASE_URL}"
python3 manager.py createdb

echo "run webserver ..."
python3 manager.py runserver --host 0.0.0.0 --port 5000 --debug \
    >"${WORK_DIR}/logs/webserver.log" 2>&1 &
sleep 10

# echo "just waiting..."
# sleep 300

cd $WORK_DIR
echo ""
echo "=========================================="
echo "Launch Chimbuko parameter server"
echo "=========================================="
echo "run parameter server ..."
bin/app/pserver 2 "${WORK_DIR}/logs/parameters.log" $NMPIS "http://0.0.0.0:5000/api/anomalydata" &
    # >"${WORK_DIR}/logs/ps.log" 2>&1 &
ps_pid=$!
sleep 5

echo ""
echo "=========================================="
echo "Launch Application with anomaly detectors"
echo "=========================================="
if [ "$ADIOS_MODE" == "SST" ]
then
    echo "Use SST mode: NWChem + AD"
    mpirun --allow-run-as-root -n $NMPIS bin/app/driver $ADIOS_MODE \
        $WORK_DIR/BP $BP_PREFIX "${WORK_DIR}/executions" "tcp://0.0.0.0:5559" ${AD_SIGMA} ${AD_WINSZ} 0 &
        # >logs/ad.log 2>&1 &
    sleep 5
    mpirun --allow-run-as-root -n $NMPIS nwchem ethanol_md.nw 
        # >logs/nwchem.log 2>&1 
else
    echo "Use BP mode"
    if ! $HAS_BPFILE
    then
        echo "Run NWChem"
        mpirun --allow-run-as-root -n $NMPIS nwchem ethanol_md.nw >logs/nwchem.log 2>&1 
    fi
    echo "Run anomaly detectors"
    mpirun --allow-run-as-root -n $NMPIS bin/app/driver $ADIOS_MODE $WORK_DIR/BP $BP_PREFIX \
        "${WORK_DIR}/executions"  "tcp://0.0.0.0:5559" ${AD_SIGMA} ${AD_WINSZ} ${AD_INTERVAL}
        # >logs/ad.log 2>&1 
fi

echo ""
echo "=========================================="
echo "Shutdown Chimbuko parameter server"
echo "=========================================="
# bin/pshutdown "tcp://0.0.0.0:5559"
echo "shutdown parameter server ..."
wait $ps_pid

sleep 30
cd $CHIMBUKO_VIS_ROOT
echo ""
echo "=========================================="
echo "Shutdown Chimbuko visualization server"
echo "=========================================="
curl -X GET http://0.0.0.0:5000/tasks/inspect
echo "shutdown webserver ..."
curl -X GET http://0.0.0.0:5000/stop
echo "shutdown celery workers ..."
pkill -9 -f 'celery worker'
echo "shutdown redis server ..."
webserver/shutdown-redis.sh

echo "Bye~~!!"
