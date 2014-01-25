#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os, sys, subprocess
from distutils.core import setup, Extension

this_dir = os.path.dirname(os.path.abspath(__file__))

def install_libsophia():
    src_dir = os.path.join(this_dir, "lib", "sophia", "db")
    os.chdir(src_dir)
    try:
        subprocess.call("make", shell=True)
        subprocess.call("make install", shell=True)
    finally:
        os.chdir(this_dir)

if "--no-lib" in sys.argv:
    sys.argv.remove("--no-lib")
else:
    try:
        install_libsophia()
    except Exception as e:
        sys.stderr.write("failed to install libsophia: %s\n" % e)
        sys.exit(1)

with open(os.path.join(this_dir, "README.rst")) as f:
	longdescr = f.read()

cmodule = Extension('_sophia', sources=["sophia/pysophia.c"], libraries=["sophia"])

setup (
    name = 'Sophia',
    version = '0.1',
    description = 'Python bindings for the Sophia database library',
    long_description = longdescr,
    author='Michaël Meyer',
    url='https://github.com/doukremt/python-sophia.git',
    ext_modules = [cmodule],
    packages = ["sophia"],
    classifiers=(
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Operating System :: OS Independent',
        'Topic :: Software Development :: Libraries :: Python Modules',
        'Programming Language :: C',
        'Programming Language :: Python',
    )
)
