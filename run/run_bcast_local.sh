#!/bin/bash
set -euo pipefail

# Build and run the local broadcast fault-injection campaign with 32 MiB caps.
# Defaults use host mpiexec (no Singularity); override via env vars below.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Tunables (override via environment)
RUNS_PER_MODE="${RUNS_PER_MODE:-3}"
USE_SINGULARITY="${USE_SINGULARITY:-0}"

# 32 MiB caps (for int buffers)
MAX_BUF_BYTES="${MAX_BUF_BYTES:-33554432}"
BCAST_BUF="${BCAST_BUF:-2097152}"      # default buffer size (2,097,152 ints ~8 MiB)

echo "==> Building broadcast binaries"
make -C "$ROOT_DIR/src/linear"
make -C "$ROOT_DIR/src/binomial"
make -C "$ROOT_DIR/src/scatter_allgather"

echo "==> Running local broadcast fault-injection campaign"
(
  cd "$ROOT_DIR/run"
  RUNS_PER_MODE="$RUNS_PER_MODE" \
  USE_SINGULARITY="$USE_SINGULARITY" \
  MAX_BUF_BYTES="$MAX_BUF_BYTES" \
  BCAST_BUF="$BCAST_BUF" \
  ./test_bcast_local.sh
)

echo "==> Done. Broadcast logs: $ROOT_DIR/log/log_bcast_*_local.csv"
