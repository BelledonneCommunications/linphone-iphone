#!/usr/bin/python

import argparse
import types
import sys

def main(argv = None):
    if argv is None:
        argv = sys.argv
    argparser = argparse.ArgumentParser(description="Generate enums documentation of the Linphone API.")
    argparser.add_argument('-o', '--outputfile', metavar='outputfile', type=argparse.FileType('w'), help="Output .rst file describing the Linphone API enums.")
    args = argparser.parse_args()
    if args.outputfile == None:
        args.outputfile = open('enums.rst', 'w')

    module = __import__('linphone', globals(), locals())

    for name in dir(module):
        if name == 'testing' or name == 'linphone':
            continue
        if type(getattr(module, name)) == types.ModuleType:
            args.outputfile.write('linphone.' + name + '\n')
            args.outputfile.write('^' * len('linphone.' + name) + '\n\n')
            args.outputfile.write(getattr(module, name).__doc__)
            args.outputfile.write('\n')

if __name__ == "__main__":
    sys.exit(main())
