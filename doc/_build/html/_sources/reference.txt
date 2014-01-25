Reference
*********

Main objects
============

.. exception:: sophia.Error

   Exception raised for all database-related errors.

.. module:: sophia
   :synopsis: Python bindings to `libsophia`

.. class:: Database

   Main `Database` class.
   
   Keys or values passed as argument to the methods which accept them are expected to be byte strings. Returned keys or values are always byte strings.

   .. method:: setopt(constant, value1[, value2])

      Set an option on this database.
      
      See the `sophia documentation <http://sphia.org/sp_ctl.html>`_ for a summary of the available options. :const:`SPDIR` and :const:`SPALLOC` are not supported.

   .. method:: open(path)

      Open the database, creating it if doesn't exist yet.
	
   .. method:: close()
   
      Close the current connection, if any.
   
   .. method:: is_closed()
   
      Is this database closed? `True` if so, `False` otherwise.

   .. method:: set(key, value)
   
      Add a record.

   .. method:: get(key[, default])
   
      Retrieve a record given its key. If it doesn't exist, return `default` if given, `None` otherwise.

   .. method:: contains(key)
   
      Is this key in the database? `True` if so, `False` otherwise.

   .. method:: begin()

      Start a transaction.

   .. method:: commit()
   
      Commit the current transaction.

   .. method:: rollback()

      Abort the current transaction.

   .. method:: len()

      How many records are there in this database?

   .. method:: iterkeys(start_key=None, order=sophia.SPGTE)

      Iterate over all the keys in this database, starting at `start_key`, and in `order`.
	
      Possible values for `order` are:

      * :const:`sophia.SPGT`  - increasing order (skipping the key, if it is equal)
      * :const:`sophia.SPGTE` - increasing order (with key)
      * :const:`sophia.SPLT`  - decreasing order (skipping the key, if it is equal)
      * :const:`sophia.SPLTE` - decreasing order

   .. method:: itervalues(start_key=None, order=sophia.SPGTE)

      Same as :meth:`Database.iterkeys()`, but for values.

   .. method:: iteritems(start_key=None, order=sophia.SPGTE)

      Same as :meth:`Database.iterkeys()`, but for pairs of (key, value).


Database models
===============

.. class:: sophia.ObjectDatabase(pack_key=<function dumps>, unpack_key=<function loads>, pack_value=<function dumps>, unpack_value=<function loads>)

   Database model for storing arbitrary kinds of objects. `pack_key`, `unpack_key`, `pack_value`, and `unpack_value`, should be callables that, when passed an object as parameter, return a byte representation of it, suitable for storage. By default, all these functions use the :mod:`pickle` module.


.. class:: sophia.ThreadedDatabase

   Thread-safe database model.

   It should only be used if you want to use a database in a threaded environment AND need to iterate over it. Otherwise, the vanilla :class:`Database` class is suitable (and more efficient).

.. class:: sophia.ThreadedObjectDatabase(pack_key=<function dumps>, unpack_key=<function loads>, pack_value=<function dumps>, unpack_value=<function loads>)

   Mixing of a :class:`ThreadedDatabase` and an :class:`ObjectDatabase`.


