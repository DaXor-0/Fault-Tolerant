#!/bin/bash
set -euo pipefail

# Local broadcast performance comparison harness.
# Runs baseline Open MPI broadcast algorithms (original/*.exe) vs fault-aware wrappers (linear/binomial/scatter_allgather main).
# Uses host mpiexec by default; set USE_SINGULARITY=1 and SINGULARITY_IMAGE to use a container.

mkdir -p ../out
mkdir -p ../log
mkdir -p ../data/data_bcast_compare

REPEATS=${REPEATS:-2}
MAX_NP=${MAX_NP:-16}
MAX_BUF=${MAX_BUF:-8388608}      # 32 MiB for int buffers
MAX_BUF_CAP=${MAX_BUF_CAP:-8388608}
USE_SINGULARITY=${USE_SINGULARITY:-auto}
SINGULARITY_IMAGE=${SINGULARITY_IMAGE:-$HOME/local/mpi-ft-ulfm.sif}

run_mpiexec() {
    local np=$1
    local exe=$2
    local buf_size=$3
    local outfile=$4

    local mpi_cmd=(mpiexec --with-ft ulfm -np "$np" "$exe" "$buf_size")
    if [[ "$USE_SINGULARITY" != "0" && -f "$SINGULARITY_IMAGE" ]]; then
        mpi_cmd=(singularity exec -B "$HOME/local" -B "$TMPDIR:$TMPDIR" "$SINGULARITY_IMAGE" "${mpi_cmd[@]}")
    fi

    "${mpi_cmd[@]}" > "$outfile"
}

run_tests() {
    local np=$1
    local buf_size=$2

    # Linear
    run_mpiexec "$np" ../src/original/linear.exe "$buf_size" ../out/original_bcast_linear.txt
    run_mpiexec "$np" ../src/linear/main "$buf_size" ../out/bcast_linear.txt
    BCAST_BASELINE_FILE=../out/original_bcast_linear.txt \
    BCAST_FT_FILE=../out/bcast_linear.txt \
    BCAST_CSV_OUT=../data/data_bcast_compare/linear.csv \
    python3 ../analysis/check_bcast_compare.py

    # Binomial
    run_mpiexec "$np" ../src/original/binomial.exe "$buf_size" ../out/original_bcast_binomial.txt
    run_mpiexec "$np" ../src/binomial/main "$buf_size" ../out/bcast_binomial.txt
    BCAST_BASELINE_FILE=../out/original_bcast_binomial.txt \
    BCAST_FT_FILE=../out/bcast_binomial.txt \
    BCAST_CSV_OUT=../data/data_bcast_compare/binomial.csv \
    python3 ../analysis/check_bcast_compare.py

    # Scatter + Allgather
    run_mpiexec "$np" ../src/original/scatter_allgather.exe "$buf_size" ../out/original_bcast_scatter_ag.txt
    run_mpiexec "$np" ../src/scatter_allgather/main "$buf_size" ../out/bcast_scatter_ag.txt
    BCAST_BASELINE_FILE=../out/original_bcast_scatter_ag.txt \
    BCAST_FT_FILE=../out/bcast_scatter_ag.txt \
    BCAST_CSV_OUT=../data/data_bcast_compare/scatter_ag.csv \
    python3 ../analysis/check_bcast_compare.py
}

for rep in $(seq 1 "$REPEATS"); do
    echo "Repeat $rep / $REPEATS"

    np=4
    while [ "$np" -le "$MAX_NP" ]; do
        buf_size=1
        while [ "$buf_size" -le "$MAX_BUF" ] && [ "$buf_size" -le "$MAX_BUF_CAP" ]; do
            run_tests "$np" "$buf_size"
            buf_size=$((buf_size * 2))
        done
        np=$((np * 2))
    done
done
