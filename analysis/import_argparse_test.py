import argparse

# parser = argparse.ArgumentParser(description='Process some integers.')
# parser.add_argument('integers', metavar='N', type=int, nargs='+',
#                    help='an integer for the accumulator')
# parser.add_argument('--sum', dest='accumulate', action='store_const',
#                    const=sum, default=max,
#                    help='sum the integers (default: find the max)')

# args = parser.parse_args()
# print args.accumulate(args.integers)


directories="."

parser = argparse.ArgumentParser(description='Read gpspipe -w -T from gpsd.log and create a csv from it.')
parser.add_argument('directories',metavar='N', type=str, nargs='+', help='optional list of diretories')

args=parser.parse_args()
print directories
