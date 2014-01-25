.. pysophia documentation master file, created by
   sphinx-quickstart on Fri Jan 24 14:00:19 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

pysophia documentation
**********************

This package provides C bindings for `sophia <http://sphia.org/>`_, a ligthweight `DBM-like <http://en.wikipedia.org/wiki/Dbm>`_ database. Available operations on a database are inserting a key-value pair, deleting it, or retrieving a value given its key. It is also possible to traverse the records of a database in ascending or descending order.

Installation
============

First install libsophia using `this script <https://raw.github.com/doukremt/python-sophia/master/install_lib.sh>`_ (to be run from the source package directory if you have the source distribution, or from `/tmp` or similar). Then install the bindings:

With pip::

    pip install sophia

With the source package::

    python setup.py install

If you want to check how the library performs, look at the `benchmarks <http://sphia.org/benchmarks.html>`_ on the website of the author, and run the script ``bench.py`` located in the ``tests`` directory of this package.

Contents
========

.. toctree::
   :maxdepth: 2
   
   tutorial
   reference


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

