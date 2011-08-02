#!/usr/bin/env python

import sys

# variable name
name = sys.argv[1];

# stdin should be a Photoshop 8-bit RGB raw file
data = sys.stdin.read()
print 'unsigned char %s[] = {' % name
for i in range(0, len(data), 12):
    chunk = data[i:i+12]
    hex = ['0x%02x' % ord(c) for c in chunk]
    print '    %s,' % ', '.join(hex)
print '};'
