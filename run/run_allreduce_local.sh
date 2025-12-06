#!/bin/bash
set -euo pipefail

# Build and run the local allreduce fault and performance campaigns with 32 MiB caps.
# Defaults use host mpiexec (no Singularity); override via env vars below.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Tunables (override via environment)
RUNS_PER_MODE="${RUNS_PER_MODE:-3}"
REPEATS="${REPEATS:-2}"
MAX_NP="${MAX_NP:-16}"
USE_SINGULARITY="${USE_SINGULARITY:-0}"

# 32 MiB caps (for int buffers)
MAX_BUF_BYTES="${MAX_BUF_BYTES:-33554432}"
MAX_BUF="${MAX_BUF:-8388608}"
MAX_BUF_CAP="${MAX_BUF_CAP:-8388608}"

echo "==> Building allreduce binaries"
make -C "$ROOT_DIR/src/rd"
make -C "$ROOT_DIR/src/raben"
make -C "$ROOT_DIR/src/original"

echo "==> Running local allreduce fault-injection campaign"
(
  cd "$ROOT_DIR/run"
  RUNS_PER_MODE="$RUNS_PER_MODE" \
  USE_SINGULARITY="$USE_SINGULARITY" \
  MAX_BUF_BYTES="$MAX_BUF_BYTES" \
  ./test_fault_local.sh
)

echo "==> Running local allreduce performance comparison"
(
  cd "$ROOT_DIR/run"
  REPEATS="$REPEATS" \
  MAX_NP="$MAX_NP" \
  USE_SINGULARITY="$USE_SINGULARITY" \
  MAX_BUF="$MAX_BUF" \
  MAX_BUF_CAP="$MAX_BUF_CAP" \
  ./test_compare_local.sh
)

echo "==> Done. Logs:"
echo "  Fault logs: $ROOT_DIR/log/log_single_*_local.csv"
echo "  Compare logs: $ROOT_DIR/data/data_compare/*.csv"
