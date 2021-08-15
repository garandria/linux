import sys, re

def read_config(config_file):
    """Read a config

    :param config_file: path to a `.config` file
    :type config_file: string
    :return: a dictionary representation of the `.config` file.
    :rtype: dict

    """
    d = dict()
    with open(config_file, 'r') as f:
        line = f.readline()
        while line:
            if not line.startswith('#') and line != '\n':
                m = re.search(r'CONFIG_(\w+)=([\w"-/]+)', line)
                d[m.group(1)] = m.group(2)
            line = f.readline()
    return d

def main():
    configs = [read_config(config) for config in sys.argv[1:]]
    merged = configs[0]

    print("Merged {}: \t{:4d} features".format(sys.argv[1],len(merged)))

    for i in range(len(configs[1:])):
        config = configs[i]
        c_in = len([feat for feat in config if feat in merged])
        print("{}: \t{:4d}/{:4d} in merged"\
              .format(sys.argv[i+2], c_in, len(config)))


if __name__ == "__main__":
    main()
