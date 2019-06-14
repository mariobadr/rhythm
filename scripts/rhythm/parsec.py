import sys
import os
import subprocess

parsec_benchmarks = {
    'blackscholes',
    'bodytrack',
    'canneal',
    'dedup',
    'facesim',
    'ferret',
    #'fluidanimate',
    #'raytrace',
    'streamcluster',
    'swaptions',
    #'vips',
    #'x264'
}


splash2x_benchmarks = {
    'splash2x.barnes',
    #'splash2x.cholesky',
    'splash2x.fft',
    #'splash2x.fmm',
    'splash2x.lu_cb',
    'splash2x.lu_ncb',
    'splash2x.ocean_cp',
    'splash2x.ocean_ncp',
    'splash2x.radiosity',
    'splash2x.radix',
    'splash2x.volrend',
    'splash2x.water_nsquared',
    'splash2x.water_spatial',
}


all_benchmarks = parsec_benchmarks.union(splash2x_benchmarks)


def find_parsecmgmt():
    parsecmgmt = os.environ.get('PARSEC_ROOT')
    if parsecmgmt is None:
        sys.exit("Error: could not find the PARSEC_ROOT environment variable.")

    parsecmgmt = os.path.join(parsecmgmt, "bin", "parsecmgmt")
    if not os.path.isfile(parsecmgmt):
        sys.exit("Error: could not find parsecmgmt at {}".format(parsecmgmt))

    return parsecmgmt


def run_benchmark(output_file, config, benchmark, input_set, thread_count, instrumentation):
    parsecmgmt = os.path.abspath(find_parsecmgmt())

    parsec_command = "{} -c {} -a run -p {} -i {} -n {}".format(parsecmgmt, config, benchmark, input_set, thread_count)

    if instrumentation is not None:
        parsec_command = "{} -s".format(parsec_command)
        parsec_command = parsec_command.split()
        parsec_command.append("{}".format(instrumentation))

    subprocess.run(parsec_command, stdout=output_file, stderr=output_file)
