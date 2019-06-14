#!/usr/bin/python3

import argparse
import subprocess
import sys
import os
import json

from datetime import datetime
from rhythm import parsec
from rhythm import gnutime


def find_pin():
    pin = os.environ.get('PIN_ROOT')
    if pin is None:
        sys.exit("Error: could not find the PIN_ROOT environment variable.")

    pin = os.path.join(pin, "pin")
    if not os.path.isfile(pin):
        sys.exit("Error: could not find pin at {}".format(pin))

    return pin


def find_output_files(start_dir, substring):
    files = []

    for root, dirnames, filenames in os.walk(start_dir):
        for file in filenames:
            if substring in os.path.basename(file):
                files.append(os.path.join(root, file))

    return files


def instrument_benchmark(pin_tool, output_dir, output_file, benchmark, input_set, thread_count):
    pin = os.path.abspath(find_pin())
    pin_tool = os.path.abspath(pin_tool)

    run_id = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
    output_dir = os.path.join(output_dir, "{}".format(run_id))

    print("Creating directory: {}".format(output_dir))
    os.makedirs(output_dir, exist_ok=False)
    output_file = os.path.abspath(os.path.join(output_dir, output_file))
    time_file = os.path.abspath(os.path.join(output_dir, "time.csv"))

    cpu_info_file = open(os.path.join(output_dir, "lscpu.txt"), "w")
    subprocess.run("lscpu", stdout=cpu_info_file)

    time_command = gnutime.get_time_command(time_file)
    pin_command = "{} -t {} -o {} --".format(pin, pin_tool, output_file)
    instrumentation = "{} {}".format(time_command, pin_command)

    print("Instrumenting {} with {}".format(benchmark, os.path.basename(pin_tool)))
    parsec_out_file = open(os.path.join(output_dir, "parsec.out"), "w")
    parsec.run_benchmark(parsec_out_file, "gcc-pthreads", benchmark, input_set, thread_count, instrumentation)

    with open(os.path.join(output_dir, "config.json"), 'w') as config_file:
        data = {
            'run-type': "pin",
            'run-id': run_id,
            'pin-tool': os.path.basename(pin_tool),
            'benchmark': benchmark,
            'input-set': input_set,
            'thread-count': thread_count
        }

        json.dump(data, config_file, indent=2)

    output_files = find_output_files(output_dir, os.path.basename(output_file))
    for file in output_files:
        print("Compressing {}".format(file))
        gzip_command = "gzip {}".format(file)
        subprocess.run(gzip_command.split())

    compressed_files = find_output_files(output_dir, os.path.basename(output_file))
    compressed_files.sort()
    with open(os.path.join(output_dir, "output-manifest.txt"), 'w') as manifest:
        manifest.write('\n'.join(compressed_files))


def main():
    p = argparse.ArgumentParser(description="Instrument the PARSEC benchmarks with a Pin tool.")
    p.add_argument('-p', '--pin-tool', dest="pin_tool", default=None)
    p.add_argument('-o', '--output-dir', dest='output_dir', default=None)
    p.add_argument('-f', '--output-file', dest='output_file', default="output.txt")
    p.add_argument('-i', '--input-set', dest='input_set', default="test")
    p.add_argument('-b', '--benchmark', dest='benchmark', default="blackscholes")
    p.add_argument('-t', '--thread-count', dest="thread_count", default=4)
    p.add_argument('-r', '--run-count', dest="run_count", default=1, type=int)

    (args) = p.parse_args()

    if args.pin_tool is None:
        sys.exit("Error: no path to pin tool.")

    if not os.path.exists(args.pin_tool):
        sys.exit("Error: could not find pin tool at {}".format(args.pin_tool))

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
            instrument_benchmark(args.pin_tool, args.output_dir, args.output_file, benchmark, args.input_set,
                                 args.thread_count)


if __name__ == "__main__":
    main()
