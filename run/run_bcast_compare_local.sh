#!/bin/bash
set -euo pipefail

# Build and run the local broadcast performance comparison with 32 MiB caps.
# Defaults use host mpiexec (no Singularity); override via env vars below.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Tunables (override via environment)
REPEATS="${REPEATS:-2}"
MAX_NP="${MAX_NP:-16}"
USE_SINGULARITY="${USE_SINGULARITY:-0}"
MAX_BUF="${MAX_BUF:-8388608}"      # 32 MiB in ints
MAX_BUF_CAP="${MAX_BUF_CAP:-8388608}"

echo "==> Building broadcast binaries"
make -C "$ROOT_DIR/src/linear"
make -C "$ROOT_DIR/src/binomial"
make -C "$ROOT_DIR/src/scatter_allgather"
make -C "$ROOT_DIR/src/original" linear binomial scatter_allgather

echo "==> Running local broadcast comparison"
(
  cd "$ROOT_DIR/run"
  REPEATS="$REPEATS" \
  MAX_NP="$MAX_NP" \
  USE_SINGULARITY="$USE_SINGULARITY" \
  MAX_BUF="$MAX_BUF" \
  MAX_BUF_CAP="$MAX_BUF_CAP" \
  ./test_bcast_compare_local.sh
)

echo "==> Done. Broadcast compare logs: $ROOT_DIR/data/data_bcast_compare/*.csv"
