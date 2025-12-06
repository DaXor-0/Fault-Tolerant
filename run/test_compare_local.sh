#!/bin/bash

# Local performance comparison harness mirroring slurm/test_compare.slurm
# but without SLURM or Singularity. Assumes ULFM-capable mpiexec in PATH.

mkdir -p ../out
mkdir -p ../log
mkdir -p ../data/data_compare

REPEATS=${REPEATS:-5}
MAX_NP=${MAX_NP:-64}
MAX_BUF=${MAX_BUF:-8388608}    # default cap: 32 MiB for 4-byte ints
MAX_BUF_CAP=${MAX_BUF_CAP:-8388608} # hard cap to avoid oversizing
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

    run_mpiexec "$np" ../src/original/rd.exe "$buf_size" ../out/original_rd.txt
    run_mpiexec "$np" ../src/rd/main "$buf_size" ../out/rd.txt

    run_mpiexec "$np" ../src/original/raben.exe "$buf_size" ../out/original_raben.txt
    run_mpiexec "$np" ../src/raben/main "$buf_size" ../out/raben.txt

    python3 ../analysis/check_compare.py
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
