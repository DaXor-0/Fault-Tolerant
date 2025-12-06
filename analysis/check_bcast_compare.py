import os
import csv

# Paths will be provided by runner; this script just compares baseline vs fault-aware outputs


def read_output(file_path):
    """Parse P/Size/Time and Hello lines from a broadcast run output."""
    np = size = time_s = None
    results = []
    with open(file_path, "r") as f:
        for line in f:
            tokens = line.split()
            if not tokens:
                continue
            if tokens[0] == "P:":
                np = int(tokens[-1])
            elif tokens[0] == "Size:":
                size = int(tokens[-1])
            elif tokens[0] == "Time:":
                try:
                    time_s = float(tokens[-1])
                except ValueError:
                    time_s = None
            elif tokens[0] == "Hello":
                try:
                    results.append(int(tokens[-1]))
                except Exception:
                    pass
    return np, size, time_s, results


def check_results(res_baseline, res_ft, np):
    """Ensure all ranks produced the same value and both variants match."""
    if res_baseline is None or res_ft is None:
        return False
    if len(res_baseline) != np or len(res_ft) != np:
        return False
    target = res_baseline[0]
    for i in range(np):
        if res_baseline[i] != target or res_ft[i] != target:
            return False
    return True


def append_csv(path, np, size, time_baseline, time_ft, result):
    headers = ["NP", "SIZE", "TIME_BASELINE", "TIME_FAULT_AWARE", "RESULT"]
    first = not os.path.exists(path)
    with open(path, "a", newline="") as f:
        writer = csv.writer(f, delimiter=";")
        if first:
            writer.writerow(headers)
        writer.writerow([np, size, time_baseline, time_ft, result])


def main():
    # Expect four files and one output CSV path via env vars (set by caller)
    baseline_file = os.environ.get("BCAST_BASELINE_FILE")
    ft_file = os.environ.get("BCAST_FT_FILE")
    csv_out = os.environ.get("BCAST_CSV_OUT")

    if not baseline_file or not ft_file or not csv_out:
        raise SystemExit("Missing env vars: BCAST_BASELINE_FILE, BCAST_FT_FILE, BCAST_CSV_OUT")

    np_b, size_b, time_b, res_b = read_output(baseline_file)
    np_f, size_f, time_f, res_f = read_output(ft_file)

    if np_b != np_f or size_b != size_f:
        raise SystemExit("Mismatch in NP/Size between baseline and fault-aware runs")

    ok = check_results(res_b, res_f, np_b)
    if not ok:
        err_path = os.environ.get("BCAST_ERROR_FILE", "../out/bcast_error.txt")
        with open(err_path, "a") as f:
            f.write(f"BASELINE: {np_b}, {size_b}, {time_b}, {res_b}\n")
            f.write(f"FAULT_AWARE: {np_f}, {size_f}, {time_f}, {res_f}\n")
            f.write("#############################################\n")
        raise SystemExit("Broadcast results mismatch; see error log")

    append_csv(csv_out, np_b, size_b, time_b, time_f, res_b[0] if res_b else None)
    print(f"OK bcast compare np={np_b} size={size_b} t_base={time_b} t_ft={time_f}")


if __name__ == "__main__":
    main()
