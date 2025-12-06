#!/bin/bash
set -euo pipefail

# Local broadcast fault-injection harness for linear, binomial, scatter+allgather.
# Uses host mpiexec by default; set USE_SINGULARITY=1 and SINGULARITY_IMAGE to use a container.

mkdir -p ../out
mkdir -p ../log

RUNS_PER_MODE=${RUNS_PER_MODE:-3}           # runs for no-fault and fault modes
MAX_BUF_BYTES=${MAX_BUF_BYTES:-33554432}    # 32 MiB cap for vectors
MAX_BUF_COUNT=$((MAX_BUF_BYTES / 4))        # assuming 4-byte ints
BCAST_BUF=${BCAST_BUF:-2097152}             # default buffer size (2,097,152 ints ~ 8 MiB)

# Clamp buffer to cap
if (( BCAST_BUF > MAX_BUF_COUNT )); then
    BCAST_BUF=$MAX_BUF_COUNT
fi

run_experiments() {
    local runs=$1
    local exe=$2
    local algo=$3
    local log_file=$4
    local np=$5
    local buf_size=$6

    # No-fault runs
    for run in $(seq 1 "$runs"); do
        TIMEOUT=30
        options=(0.8 1 1.2 1.5 1.7 2 2.2)
        delay=${options[$RANDOM % ${#options[@]}]}

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $delay" >> ../out/test_log.txt
        echo "BUF_SIZE = $buf_size" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        {
            time ./run_mpi.sh "$np" "$delay" "$buf_size" "$TIMEOUT" "0" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 ../analysis/check_bcast_fault.py "$algo" "$log_file"

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo "$np $run $algo NoFault"
        sleep 1
    done

    # Single-fault runs; only count successful single-kill runs
    count=1
    while [ "$count" -le "$runs" ]; do
        TIMEOUT=30
        options=(0.8 1 1.2 1.5 1.7 2 2.2)
        delay=${options[$RANDOM % ${#options[@]}]}

        echo "Generated values:" > ../out/test_log.txt
        echo "N = $np" >> ../out/test_log.txt
        echo "DELAY = $delay" >> ../out/test_log.txt
        echo "BUF_SIZE = $buf_size" >> ../out/test_log.txt
        echo "TIMEOUT = $TIMEOUT" >> ../out/test_log.txt

        {
            time ./run_mpi.sh "$np" "$delay" "$buf_size" "$TIMEOUT" "1" "$exe"
        } >> ../out/test_log.txt 2>&1

        python3 ../analysis/check_bcast_fault.py "$algo" "$log_file"
        check=$(<"../out/check.txt")

        if [[ "$check" == "True" ]]; then
            count=$((count + 1))
        fi

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt ../out/check.txt

        echo "$np $count $algo Fault"
        sleep 1
    done
}

# Default matrix: NP in {4,8,16,32}, buffer size clamped to 32 MiB cap.
run_experiments "$RUNS_PER_MODE" ../src/linear/main           Linear    ../log/log_bcast_linear_local.csv           4  "$BCAST_BUF"
run_experiments "$RUNS_PER_MODE" ../src/binomial/main         Binomial  ../log/log_bcast_binomial_local.csv         4  "$BCAST_BUF"
run_experiments "$RUNS_PER_MODE" ../src/scatter_allgather/main ScatterAG ../log/log_bcast_scatter_ag_local.csv      4  "$BCAST_BUF"

run_experiments "$RUNS_PER_MODE" ../src/linear/main           Linear    ../log/log_bcast_linear_local.csv           8  "$BCAST_BUF"
run_experiments "$RUNS_PER_MODE" ../src/binomial/main         Binomial  ../log/log_bcast_binomial_local.csv         8  "$BCAST_BUF"
run_experiments "$RUNS_PER_MODE" ../src/scatter_allgather/main ScatterAG ../log/log_bcast_scatter_ag_local.csv      8  "$BCAST_BUF"

# run_experiments "$RUNS_PER_MODE" ../src/linear/main           Linear    ../log/log_bcast_linear_local.csv           16 "$BCAST_BUF"
# run_experiments "$RUNS_PER_MODE" ../src/binomial/main         Binomial  ../log/log_bcast_binomial_local.csv         16 "$BCAST_BUF"
# run_experiments "$RUNS_PER_MODE" ../src/scatter_allgather/main ScatterAG ../log/log_bcast_scatter_ag_local.csv      16 "$BCAST_BUF"
#
# run_experiments "$RUNS_PER_MODE" ../src/linear/main           Linear    ../log/log_bcast_linear_local.csv           32 "$BCAST_BUF"
# run_experiments "$RUNS_PER_MODE" ../src/binomial/main         Binomial  ../log/log_bcast_binomial_local.csv         32 "$BCAST_BUF"
# run_experiments "$RUNS_PER_MODE" ../src/scatter_allgather/main ScatterAG ../log/log_bcast_scatter_ag_local.csv      32 "$BCAST_BUF"
