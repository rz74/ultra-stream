#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail
trap 'echo "[ERROR] Script failed at line $LINENO"; exit 1' ERR

# ======= CONFIGURATION =======
TCP_PORT=6010
LOG_DIR=logs
MCAST_IP="239.0.0.1"
MCAST_PORT=5000
INTERFACE="eth0"
TCP_HOST="127.0.0.1"
NUM_PACKETS=1000
TEST_DATA_DIR="test_data"
# ==============================

echo "[PROMPT] Do you want to rebuild the project binaries? (y/n)"
read -r REBUILD

if [[ "$REBUILD" == "y" || "$REBUILD" == "Y" ]]; then
    echo "========== STEP 0: Build all project binaries =========="
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(nproc)
    cd ..

    echo "[INFO] Building generate_test_vectors..."
    g++ -std=c++11 -Iinclude \
        tools/generate_test_vectors.cpp \
        src/data_book.cpp \
        src/composite_score_calculator.cpp \
        src/parser_utils.cpp \
        -o build/generate_test_vectors
else
    echo "[INFO] Skipping rebuild step."
fi

echo "========== STEP 1: Generating Test Vectors =========="
mkdir -p ${TEST_DATA_DIR}
rm -f ${TEST_DATA_DIR:?}/*
echo "[INFO] Running generate_test_vectors with $NUM_PACKETS packets..."
./build/generate_test_vectors $TEST_DATA_DIR $NUM_PACKETS
echo "[INFO] Output written to $TEST_DATA_DIR"

echo "========== STEP 2: Running Full System Test =========="
mkdir -p $LOG_DIR
mkdir -p test_results
rm -f test_results/*

PID_ON_PORT=$(lsof -t -i TCP:$TCP_PORT || true)
if [ -n "$PID_ON_PORT" ]; then
    echo "[INFO] Killing previous process(es) on port $TCP_PORT:"
    echo "$PID_ON_PORT" | xargs -r kill -9
fi

echo "[INFO] Launching tcp_receiver on port $TCP_PORT..."
setsid ./build/bin/tcp_receiver $TCP_PORT \
  > $LOG_DIR/tcp_stdout.log \
  2> $LOG_DIR/tcp_stderr.log &

TCP_PID=$!
sleep 0.2

echo "[INFO] Running system test"
mkdir -p test_results
./build/bin/test_all "$MCAST_IP" "$MCAST_PORT" "$INTERFACE" "$TCP_HOST" "$TCP_PORT" 2>&1 | tee test_results/test_all.log

status=$?

echo "[INFO] Killing tcp_receiver with PID=$TCP_PID"
if ps -p $TCP_PID > /dev/null; then
    kill $TCP_PID
    wait $TCP_PID 2>/dev/null || true
fi


wait $TCP_PID 2>/dev/null || true

if [ "$status" -eq 0 ]; then
    echo "[INFO] test_all FINISHED"
else
    echo "[INFO] test_all ABORTED"
    echo "[INFO] See $LOG_DIR/tcp_stderr.log for receiver output"
fi

echo "[INFO] Running latency analysis..."
python3 test/evaluate_results.py || echo "[WARN] Latency report generation failed"

exit $status
