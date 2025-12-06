#!/bin/bash

# Local fault-injection harness that mirrors slurm/test_fault.slurm
# but runs directly on the host (no Singularity or SLURM required).
# Requires an ULFM-capable mpiexec in PATH.

mkdir -p ../out
mkdir -p ../log

RUNS_PER_MODE=${RUNS_PER_MODE:-5} # default runs per no-fault and fault modes
MAX_BUF_BYTES=${MAX_BUF_BYTES:-33554432} # 32 MiB cap for vectors
MAX_BUF_COUNT=$((MAX_BUF_BYTES / 4))      # assuming 4-byte ints

run_experiments() {
    local runs=$1
    local exe=$2
    local allreduce_type=$3
    local log_file=$4
    local np=$5
    local buf_size=$6

    # Enforce max buffer count
    if (( buf_size > MAX_BUF_COUNT )); then
        buf_size=$MAX_BUF_COUNT
    fi

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

        python3 ../analysis/check_fault.py "$allreduce_type" "$log_file"

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt

        echo "$np $run $allreduce_type NoFault"
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

        python3 ../analysis/check_fault.py "$allreduce_type" "$log_file"
        check=$(<"../out/check.txt")

        if [[ "$check" == "True" ]]; then
            count=$((count + 1))
        fi

        rm -f ../out/mpi_out.txt ../out/docker_out.txt ../out/test_log.txt ../out/check.txt

        echo "$np $count $allreduce_type Fault"
        sleep 1
    done
}

# Default matrix mirrors slurm/test_fault.slurm; adjust RUNS_PER_MODE to scale.
run_experiments "$RUNS_PER_MODE" ../src/rd/main RD ../log/log_single_RD_local.csv 4 187205409
run_experiments "$RUNS_PER_MODE" ../src/raben/main Raben ../log/log_single_Raben_local.csv 4 187228224

run_experiments "$RUNS_PER_MODE" ../src/rd/main RD ../log/log_single_RD_local.csv 8 133742920
run_experiments "$RUNS_PER_MODE" ../src/raben/main Raben ../log/log_single_Raben_local.csv 8 133732254

run_experiments "$RUNS_PER_MODE" ../src/rd/main RD ../log/log_single_RD_local.csv 16 110123560
run_experiments "$RUNS_PER_MODE" ../src/raben/main Raben ../log/log_single_Raben_local.csv 16 110124421

run_experiments "$RUNS_PER_MODE" ../src/rd/main RD ../log/log_single_RD_local.csv 32 56732120
run_experiments "$RUNS_PER_MODE" ../src/raben/main Raben ../log/log_single_Raben_local.csv 32 63045188
