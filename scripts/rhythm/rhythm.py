import csv
import json
import os
import sys


def read_database_file(file):
    with open(file, 'r') as database_file:
        database = json.load(database_file)

        return database


def filter_analysis_type(database, analysis_type):
    vtune_data = database["experiments"]["vtune"]
    configs = []

    for run in vtune_data:
        if run["vtune-analysis"] == analysis_type:
            configs.append(run)

    return configs


def are_matching(run1, run2):
    if run1["benchmark"] == run2["benchmark"] and run1["input-set"] == run2["input-set"] \
            and run1["thread-count"] == run2["thread-count"]:
        return True

    return False


def find_matching_trace(database, source):
    pin_data = database["experiments"]["pin"]

    for run in pin_data:
        if run["pin-tool"] == "pthread-trace.so":
            if are_matching(run, source):
                manifest = os.path.join(run["path"], "output-manifest.txt")

                if not os.path.exists(manifest):
                    sys.exit("Error: could not find {}".format(manifest))

                return manifest


def calculate_average(l):
    average = sum(l) / float(len(l))

    return int(average)


def is_valid_thread(thread_name):
    if "mkdir (" in thread_name:
        return False

    if "dash (" in thread_name:
        return False

    if "sh (" in thread_name:
        return False

    return True


def create_architecture_config(config, out):
    data_file = os.path.join(config["path"], "vtune-hotspots.csv")

    if not os.path.exists(data_file):
        sys.exit("Error: could not find {}".format(data_file))

    core_type_data = {}
    frequency_data = []

    # Extract the required data from the VTune csv file.
    with open(data_file, 'r') as csv_file:
        reader = csv.DictReader(csv_file)

        for row in reader:
            if not is_valid_thread(row["Thread"]):
                continue

            thread_id = int(row["TID"])
            cpi_rate = float(row["CPI Rate"])
            frequency = int(float(row["Average CPU Frequency"]))

            core_type_data[thread_id] = {'tid': thread_id, 'cpi.rate': cpi_rate}
            frequency_data.append(frequency)

    # Create the architectural configuration skeleton.
    arch_config = {
        'source': config,
        'architecture': {
            'core.types': [{
                'id': "default",
                'frequency.levels': [{
                    'id': 0,
                    'frequency': calculate_average(frequency_data)
                }],
                "threads": [],
            }],
            "cores": []
        },
        'system': {
            "static.frequencies": []
        }
    }

    # Populate the skeleton with TIDs and CPI rates.
    new_tid = 0
    for key in sorted(core_type_data.keys()):
        arch_config["architecture"]["core.types"][0]["threads"].append({
            'tid': new_tid,
            'cpi.rate': core_type_data[key]["cpi.rate"]
        })

        arch_config["system"]["static.frequencies"].append({
            'tid': new_tid,
            'level': 0
        })

        new_tid = new_tid + 1

    thread_count = int(config["thread-count"])
    for i in list(range(thread_count)):
        arch_config["architecture"]["cores"].append("default")

    with open(out, 'w') as output_file:
        json.dump(arch_config, output_file, indent=2)
