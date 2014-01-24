This package provides Python bindings for `sophia <http://sphia.org/>`_, a ligthweight `DBM-like <http://en.wikipedia.org/wiki/Dbm>`_ database. Available operations on a database are inserting a key-value pair, deleting it, or retrieving a value given its key. It is also possible to traverse the records of a database in ascending or descending order.

Installation
============

With pip::

    pip install sophia

With the source package::

    python setup.py install

If you want to check how the library performs, look at the `benchmarks <http://sphia.org/benchmarks.html>`_ on the website of the author, and run the script ``bench.py`` located in the ``tests`` directory of this package.
