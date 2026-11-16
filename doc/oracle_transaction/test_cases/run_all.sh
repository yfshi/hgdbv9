#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON="${PYTHON:-python3}"

"$PYTHON" "$DIR/oracle_kingbase_tx_compare.py" | tee "${TX_COMPARE_LOG:-/tmp/tx_compare_run.log}"
"$PYTHON" "$DIR/oracle_kingbase_tx_supplement.py" | tee "${TX_SUPP_LOG:-/tmp/tx_supp_run.log}"

echo "results:"
echo "  ${TX_RESULT_FILE:-/tmp/tx_compare_results.json}"
echo "  ${TX_SUPP_RESULT_FILE:-/tmp/tx_supp_results.json}"
