#!/usr/bin/python3

import argparse
import gzip
import json
import os
import sys

from rhythm import rhythm


def collect_gnu_time(database, output_filename):
    with open(output_filename, 'w') as output:
        output.write("benchmark,input-set,thread-count,run-type,run-id,gnutime\n")

        for experiment_type in database["experiments"]:
            for run in database["experiments"][experiment_type]:
                time_file = os.path.join(run["path"], "time.csv")

                if not os.path.isfile(time_file):
                    continue

                with open(time_file, 'r') as csv_file:
                    line = csv_file.readline()
                    elements = line.split(',')

                config_file = os.path.join(run["path"], "config.json")
                with open(config_file, 'r') as json_file:
                    config = json.load(json_file)

                run_type = config["run-type"]
                if run_type == "pin":
                    run_type = config["pin-tool"]

                output.write("{},{},{},{},{},{}".format(config["benchmark"], config["input-set"],
                                                        config["thread-count"], run_type, config["run-id"],
                                                        elements[0]))
                output.write("\n")


def collect_vtune_thread_time(database, output_filename):
    with open(output_filename, 'w') as output:
        output.write("benchmark,input-set,thread-count,run-type,run-id,")
        output.write("thread-id,effective-time,wait-time,wait-count")
        output.write("\n")

        for run in database["experiments"]["vtune"]:
            analysis_type = run["vtune-analysis"]
            if analysis_type != "threading":
                continue

            config_file = os.path.join(run["path"], "config.json")
            with open(config_file, 'r') as json_file:
                config = json.load(json_file)

            vtune_file = os.path.join(run["path"], "vtune-hotspots.csv")
            with open(vtune_file, 'r') as csv_file:
                csv_file.__next__()  # skip the header

                for line in csv_file:
                    elements = line.split(',')

                    if len(elements) < 18:
                        print("Not enough elements found. Skipping row.")
                        continue

                    if not rhythm.is_valid_thread(elements[0]):
                        print("Thread name {} was invalid. Skipping row.".format(elements[0]))
                        continue

                    effective_time = 0
                    try:
                        effective_time = float(elements[1])
                    except ValueError:
                        print("Could not convert {} to float.".format(elements[1]))

                    wait_time = 0
                    try:
                        wait_time = float(elements[10])
                    except ValueError:
                        print("Could not convert {} to float.".format(elements[10]))

                    tid = int(elements[18])
                    wait_count = int(elements[16])

                    output.write("{},{},{},{},{},".format(config["benchmark"], config["input-set"],
                                                          config["thread-count"], config["run-type"], config["run-id"]))
                    output.write("{},{},{},{}".format(tid, effective_time, wait_time, wait_count))
                    output.write("\n")


def collect_rhythm_thread_time(database, output_filename):
    with open(output_filename, 'w') as output:
        output.write("benchmark,input-set,thread-count,run-type,run-id,")
        output.write("thread-id,status,time")
        output.write("\n")

        for run in database["experiments"]["rhythm"]:
            config_file = os.path.join(run["path"], "config.json")
            with open(config_file, 'r') as json_file:
                config = json.load(json_file)

            rhythm_file = os.path.join(run["path"], "rhythm-time-stacks.csv")
            with open(rhythm_file, 'r') as csv_file:
                csv_file.__next__()  # skip the header

                for line in csv_file:
                    elements = line.split(',')

                    output.write("{},{},{},{},{},".format(config["benchmark"], config["input-set"],
                                                          config["thread-count"], config["run-type"], config["run-id"]))

                    tid = int(elements[0])
                    time = float(elements[2])
                    output.write("{},{},{}".format(tid, elements[1], time))
                    output.write("\n")


def collect_thread_count(database, output_filename):
    with open(output_filename, 'w') as output:
        output.write("benchmark,input-set,thread-count,run-type,run-id,")
        output.write("thread-id,event,count")
        output.write("\n")

        for run in database["experiments"]["pin"]:
            pin_tool = run["pin-tool"]
            if pin_tool != "pthread-count.so":
                continue

            config_file = os.path.join(run["path"], "config.json")
            with open(config_file, 'r') as json_file:
                config = json.load(json_file)

            count_file = os.path.join(run["path"], "pthread-count.csv.gz")
            with gzip.open(count_file, 'rt') as csv_file:
                csv_file.__next__()  # skip the header

                for line in csv_file:
                    elements = line.split(',')

                    output.write("{},{},{},{},{},".format(config["benchmark"], config["input-set"],
                                                          config["thread-count"], config["run-type"], config["run-id"]))

                    output.write("{},{},{}".format(int(elements[0]), elements[1], int(elements[2])))
                    output.write("\n")


def main():
    p = argparse.ArgumentParser(description="Create an architectural configuration based on Vtune data.")
    p.add_argument('-d', '--database-file', dest='database_file', default=None)
    p.add_argument('-o', '--output-file', dest='output_file', default="stats.csv")
    p.add_argument('-t', '--type', dest='type', default=None)

    (args) = p.parse_args()

    if args.database_file is None:
        sys.exit("Error: no path to database file.")

    if args.type is None:
        sys.exit("Error: No collection type was specified.")

    if not os.path.exists(args.database_file):
        sys.exit("Error: database file does not exist.")

    print("Reading database {}.".format(args.type))
    database = rhythm.read_database_file(args.database_file)

    if args.type == "gnu-time":
        print("Collecting gnu-time data.")
        collect_gnu_time(database, args.output_file)
    elif args.type == "vtune-thread-time":
        print("Collecting vtune-thread-time data.")
        collect_vtune_thread_time(database, args.output_file)
    elif args.type == "event-count":
        print("Collecting event-count data.")
        collect_thread_count(database, args.output_file)
    elif args.type == "rhythm-thread-time":
        print("Collecting rhythm-thread-time data.")
        collect_rhythm_thread_time(database, args.output_file)


if __name__ == "__main__":
    main()
