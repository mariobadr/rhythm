#!/usr/bin/python3

import argparse
import subprocess
import sys
import os
import json

from datetime import datetime
from rhythm import parsec
from rhythm import gnutime


def run_benchmark(output_dir, benchmark, input_set, thread_count):
    run_id = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    output_dir = os.path.join(output_dir, "{}".format(run_id))

    print("Creating directory: {}".format(output_dir))
    os.makedirs(output_dir, exist_ok=False)
    time_file = os.path.abspath(os.path.join(output_dir, "time.csv"))

    cpu_info_file = open(os.path.join(output_dir, "lscpu.txt"), "w")
    subprocess.run("lscpu", stdout=cpu_info_file)

    time_command = gnutime.get_time_command(time_file)
    parsec_out_file = open(os.path.join(output_dir, "parsec.out"), "w")
    parsec.run_benchmark(parsec_out_file, "gcc-pthreads", benchmark, input_set, thread_count, time_command)

    with open(os.path.join(output_dir, "config.json"), 'w') as config_file:
        data = {
            'run-type': "gnutime",
            'run-id': run_id,
            'benchmark': benchmark,
            'input-set': input_set,
            'thread-count': thread_count
        }

        json.dump(data, config_file, indent=2)


def main():
    p = argparse.ArgumentParser(description="Run the PARSEC benchmarks with /usr/bin/time.")
    p.add_argument('-o', '--output-dir', dest='output_dir', default=None)
    p.add_argument('-i', '--input-set', dest='input_set', default="test")
    p.add_argument('-b', '--benchmark', dest='benchmark', default="blackscholes")
    p.add_argument('-t', '--thread-count', dest="thread_count", default=4)
    p.add_argument('-r', '--run-count', dest="run_count", default=1, type=int)

    (args) = p.parse_args()

    if args.output_dir is None:
        sys.exit("Error: no path to output directory.")
    else:
        os.makedirs(args.output_dir, exist_ok=True)

    if args.benchmark == "parsec":
        benchmarks = parsec.parsec_benchmarks
    elif args.benchmark == "splash2x":
        benchmarks = parsec.splash2x_benchmarks
    elif args.benchmark == "all":
        benchmarks = parsec.all_benchmarks
    else:
        benchmarks = {args.benchmark}

    for run in list(range(args.run_count)):
        for benchmark in sorted(benchmarks):
            print("Run {} of {} for the {} benchmark.".format(run + 1, args.run_count, benchmark))
            run_benchmark(args.output_dir, benchmark, args.input_set, args.thread_count)


if __name__ == "__main__":
    main()
