This package provides Python bindings for `sophia <http://sphia.org/>`_, a ligthweight `DBM-like <http://en.wikipedia.org/wiki/Dbm>`_ database. Available operations on a database are inserting a key-value pair, deleting it, or retrieving a value given its key. It is also possible to traverse the records of a database in ascending or descending order.

Installation
============

First install libsophia using `this script <https://raw.github.com/doukremt/python-sophia/master/install_lib.sh>`_ (to be run from the source package directory if you have the source distribution, or from `/tmp` or similar). Then install the bindings:

With pip::

    pip install sophia

With the source package::

    python setup.py install

You can consult the online documentation of this package `here <http://python-sophia.readthedocs.org/en/latest/>`_. It is also present in the source package under ``doc/_build/html/``

If you want to check how the library performs, look at the `benchmarks <http://sphia.org/benchmarks.html>`_ on the website of the author, and run the script ``bench.py`` located in the ``tests`` directory of this package.
