#!/bin/bash

N=$1
DELAY=$2 
BUF_SIZE=$3
TIMEOUT=$4
KILL_VALUE=$5 
EXE_PATH=$6
KILL=$((RANDOM % (N - 1) + 1))
USE_SINGULARITY=${USE_SINGULARITY:-auto}
SINGULARITY_IMAGE=${SINGULARITY_IMAGE:-$HOME/local/mpi-ft-ulfm.sif}

# Based on the KILL_VALUE, kill one, more than one or no one
if [[ "$KILL_VALUE" == "0" ]]; then 
    echo "Kill not enabled"
    KILL=0
elif [[ "$KILL_VALUE" == "1" ]]; then
    echo "Single Kill enabled"
    KILL=1
else 
    echo "Multiple Kill enabled"
    KILL=$((RANDOM % (N - 1) + 1))
fi

# Build the mpiexec command; default to container if available, otherwise host mpiexec
mpi_cmd=(mpiexec --with-ft ulfm -np "$N" "./$EXE_PATH" "$BUF_SIZE")
if [[ "$USE_SINGULARITY" != "0" && -f "$SINGULARITY_IMAGE" ]]; then
    echo "Using Singularity image: $SINGULARITY_IMAGE"
    mpi_cmd=(singularity exec -B "$HOME/local" -B "$TMPDIR:$TMPDIR" "$SINGULARITY_IMAGE" "${mpi_cmd[@]}")
else
    echo "Running with host mpiexec (no Singularity)"
fi

# Run the executable and the kill script
timeout "$TIMEOUT" "${mpi_cmd[@]}" > ../out/mpi_out.txt &
    ./kill_procs.sh "$DELAY" "$KILL" > ../out/docker_out.txt &

wait
