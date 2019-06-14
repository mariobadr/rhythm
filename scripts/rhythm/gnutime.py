

def get_time_command(output_file):
    time_command = "/usr/bin/time -f \"%e,%U,%S,%K,%M,%D,%F,%R,%W,%c,%w\" -o {}".format(output_file)

    return time_command
