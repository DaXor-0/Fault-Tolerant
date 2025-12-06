import os
import csv
import sys


def parse_time_token(token):
    """
    Parse a GNU time-like token such as '0m1.234s' or '0m1,234s' or plain '1.234'
    into seconds (float). Falls back to None on failure.
    """
    try:
        t = token.strip().rstrip("s").replace(",", ".")
        if "m" in t:
            mins, secs = t.split("m", 1)
            return int(mins) * 60 + float(secs)
        return float(t)
    except Exception:
        return None


def append_with_separator(algo, input_file1, input_file2, input_file3, text, output_file):
    """Append detailed logs when errors occur."""
    if not os.path.exists(output_file):
        open(output_file, "w").close()

    with open(input_file1, "r") as inp1, open(input_file2, "r") as inp2, open(input_file3, "r") as inp3, open(output_file, "a") as out:
        out.write(f"Algo Used: {algo}\n")
        out.write(inp1.read())
        out.write("\n")
        out.write(inp2.read())
        out.write("\n")
        out.write(inp3.read())
        out.write("\n" + text)
        out.write("\n" + "######################################################################")


def calc_expected_res(buf_size, root_value=1):
    """
    Expected checksum printed by fault_common.c: sum(buffer[i] % 17) over the vector.
    With root_value=1 (root rank 0), the buffer is filled with 1 and result is buf_size.
    """
    return buf_size * (root_value % 17)


def mpi_output(nprocs, expected_result):
    """Parse mpi_out.txt to count survivors, detect correctness, and extract timing."""
    right_result = True
    time_s = None
    survivors = set()
    with open("../out/mpi_out.txt", "r") as file:
        lines = file.readlines()
    for line in lines:
        tokens = line.split()
        if not tokens:
            continue
        if tokens[0] == "Hello":
            try:
                survivors.add(int(tokens[2]))
                res_val = int(tokens[-1])
                if res_val != expected_result:
                    right_result = False
            except Exception:
                right_result = False
        elif tokens[0] == "Time:":
            try:
                time_s = float(tokens[-1])
            except Exception:
                time_s = None
    killed = [i for i in range(nprocs) if i not in survivors]
    return len(killed), right_result, time_s


def get_parameters():
    deadlock = False
    time_t = None
    time_val = None
    segfault = False
    abort = False
    right_result = False
    with open("../out/test_log.txt", "r") as file:
        lines = file.readlines()
    for line in lines:
        if "Segmentation fault" in line or "(core dumped)" in line:
            segfault = True
        tokens = line.split()
        if tokens:
            if tokens[0] == "N":
                nprocs = int(tokens[-1])
            elif tokens[0] == "BUF_SIZE":
                buf_size = int(tokens[-1])
            elif tokens[0] == "DELAY":
                delay = float(tokens[-1])
            elif tokens[0] == "TIMEOUT":
                timeout = int(tokens[-1])
            elif tokens[0] == "real":
                time_t = parse_time_token(tokens[-1])
            elif tokens[0] == "MPI_ABORT" or "MPI_ERRORS_ARE_FATAL" in tokens:
                abort = True

    expected = calc_expected_res(buf_size, root_value=1)
    killed, right_result, time_val = mpi_output(nprocs, expected)
    if not time_val:
        time_val = time_t
    if time_t and timeout and time_t > timeout:
        deadlock = True

    with open("../out/check.txt", "w") as f:
        if killed == 1 and right_result and not deadlock:
            f.write("True")
        else:
            f.write("False")

    return [nprocs, delay, buf_size, killed, time_val, deadlock, segfault, abort, right_result]


def main():
    algo = sys.argv[1]
    log_file = sys.argv[2]
    parameters = get_parameters()
    print(parameters)

    if parameters[5] or not parameters[8]:
        append_with_separator(algo, "../out/mpi_out.txt", "../out/docker_out.txt", "../out/test_log.txt", str(parameters), "../out/log_errors_bcast.txt")
        print("########################### ERROR ###########################")

    headers = ["N", "DELAY", "BUF SIZE", "KILLED", "TIME", "DEADLOCK", "SEGFAULT", "ABORT", "RIGHT RESULT"]
    if not os.path.exists(log_file):
        with open(log_file, "w", newline="") as file:
            writer = csv.writer(file, delimiter=";")
            writer.writerow(headers)

    with open(log_file, "a", newline="") as file:
        writer = csv.writer(file, delimiter=";")
        writer.writerow(parameters)


if __name__ == "__main__":
    main()
