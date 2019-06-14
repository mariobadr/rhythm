#!/usr/bin/python3

import argparse
import json
import os
import subprocess
import sys
import time

from datetime import datetime

from rhythm import gnutime
from rhythm import rhythm


def fix_time_file(file):
    with open(file, 'r') as time_file:
        line = time_file.read()

    line = line.replace("\"", "")

    with open(file, 'w') as time_file:
        time_file.write(line)


def run_rhythm(rhythm_exe, vtune_data, trace_file, output_dir, benchmark, thread_count, input_set):
    run_id = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    output_dir = os.path.join(output_dir, "{}".format(run_id))

    print("Creating directory: {}".format(output_dir))
    os.makedirs(output_dir, exist_ok=False)

    config_file = os.path.join(output_dir, "arch-config.json")
    rhythm.create_architecture_config(vtune_data, config_file)
    print("Generated architectural configuration.")

    with open(os.path.join(output_dir, "lscpu.txt"), 'w') as cpu_info_file:
        subprocess.run("lscpu", stdout=cpu_info_file)

    time_file = os.path.abspath(os.path.join(output_dir, "time.csv"))
    time_command = gnutime.get_time_command(time_file)
    rhythm_command = "{} -c {} -t {} -o {}".format(rhythm_exe, config_file, trace_file, output_dir)
    command = "{} {}".format(time_command, rhythm_command)

    with open(os.path.join(output_dir, "rhythm.log"), 'w') as rhythm_log_file:
        subprocess.run(command.split(), stdout=rhythm_log_file, stderr=rhythm_log_file)

    with open(os.path.join(output_dir, "config.json"), 'w') as config_file:
        data = {
            'run-type': "rhythm",
            'run-id': run_id,
            'benchmark': benchmark,
            'input-set': input_set,
            'thread-count': thread_count
        }

        json.dump(data, config_file, indent=2)

    # Hack to remove quotes from beginning and end of line
    fix_time_file(time_file)

    # Sleep for one second to avoid duplicate run IDs
    time.sleep(1)


def main():
    p = argparse.ArgumentParser(description="Create an architectural configuration based on Vtune data.")
    p.add_argument('-d', '--database-file', dest='database_file', default=None)
    p.add_argument('-o', '--output-dir', dest='output_dir', default=None)
    p.add_argument('-x', '--executable', dest='executable', default=None)
    p.add_argument('-b', '--benchmark', dest='benchmark', default=None)
    p.add_argument('-t', '--thread-count', dest='thread_count', default=None, type=int)

    (args) = p.parse_args()

    if args.executable is None:
        sys.exit("Error: no path to executable.")

    if not os.path.exists(args.executable):
        sys.exit("Error: executable does not exist.")

    if args.database_file is None:
        sys.exit("Error: no path to database file.")

    if not os.path.exists(args.database_file):
        sys.exit("Error: database file does not exist.")

    if args.output_dir is None:
        sys.exit("Error: no path to output directory.")
    else:
        os.makedirs(args.output_dir, exist_ok=True)

    database = rhythm.read_database_file(args.database_file)

    # The needed data can be found in the uarch-exploration vtune profiles.
    configs = rhythm.filter_analysis_type(database, "uarch-exploration")

    for config in configs:
        benchmark = config["benchmark"]
        if args.benchmark is not None and benchmark != args.benchmark:
            continue

        thread_count = int(config["thread-count"])
        if args.thread_count is not None and thread_count != args.thread_count:
            continue

        print("Estimating {} with {} threads".format(benchmark, thread_count))
        trace_file = rhythm.find_matching_trace(database, config)
        print("Using trace: {}".format(trace_file))

        run_rhythm(args.executable, config, trace_file, args.output_dir, benchmark, thread_count, config["input-set"])


if __name__ == "__main__":
    main()
