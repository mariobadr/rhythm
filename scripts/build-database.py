#!/usr/bin/python3

import argparse
import json
import os
import sys

from datetime import datetime


def is_valid_config(file):
    if file == "config.json":
        return True

    return False


def find_config_files(start_dir):
    files = []

    for root, dirnames, filenames in os.walk(start_dir):
        for file in filenames:
            if is_valid_config(file):
                files.append(os.path.join(root, file))

    return files


def read_config_file(file):
    with open(file, 'r') as config_file:
        config = json.load(config_file)

        return config


def build_database(input_dir):
    database = {
        'version': 1.0,
        'path': input_dir,
        'timestamp': datetime.now().strftime("%Y-%m-%d-%H-%M-%S"),
        'experiments': {}
    }

    config_files = find_config_files(input_dir)
    for config_file in config_files:
        config = read_config_file(config_file)

        # Save the path to the experiment.
        config["path"] = os.path.dirname(config_file)

        # Create different JSON arrays in the database for each run-type.
        key = config["run-type"]
        if key not in database["experiments"]:
            database["experiments"][key] = []

        database["experiments"][key].append(config)

    return database


def main():
    p = argparse.ArgumentParser(description="Create a database of all experiments found in a directory.")
    p.add_argument('-i', '--input-dir', dest='input_dir', default=None)
    p.add_argument('-o', '--output-file', dest='output_file', default="database.json")

    (args) = p.parse_args()

    if args.input_dir is None:
        sys.exit("Error: no path to traces directory.")

    database = build_database(args.input_dir)

    with open(os.path.abspath(args.output_file), 'w') as database_file:
        json.dump(database, database_file, indent=2)


if __name__ == "__main__":
    main()
