import subprocess
import sys
import os


def find_vtune():
    vtune_root = os.environ.get('VTUNE_ROOT')
    if vtune_root is None:
        sys.exit("Error: could not find the VTUNE_ROOT environment variable.")

    vtune_exe = os.path.abspath(os.path.join(vtune_root, "bin64", "amplxe-cl"))
    if not os.path.isfile(vtune_exe):
        sys.exit("Error: could not find vtune at {}".format(vtune_exe))

    return vtune_exe


def generate_report(vtune, report_type, data_dir, output_file):
    vtune_command = "{} -format=csv -csv-delimiter=comma -group-by=thread -R {} -r={} -report-output={}"\
        .format(vtune, report_type, data_dir, output_file)

    subprocess.run(vtune_command.split())
