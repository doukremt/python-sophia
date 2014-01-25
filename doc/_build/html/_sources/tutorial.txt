Tutorial
********

All examples below assume Python 2. To run them under Python 3, replace the ``"strings"`` expressions by ``b"byte strings"``.

Basics
======

A database consists in a directory containing a set of files, which is represented in Python by the :class:`sophia.Database` class. Databases are opened and closed respectively with the :meth:`Database.open()` and :meth:`Database.close()` methods::

    import sophia
    
    # create the object
    db = sophia.Database()
    
    # open the database, creating it if it does'nt exist yet.
    db.open("mydb")
    
    # close it
    db.close()

Database objects are implicitly closed when they go out of scope, but you should not rely on this behaviour, and always close them explicitly when you're done with them.

Database-related errors all raise a :class:`sophia.Error` exception. You can catch it with typical ``try``... ``except``... ``finally`` statements::

    db = sophia.Database()
    try:
        db.open("mydb")
        # do something with `db`
    except sophia.Error as e:
        print("Error: %s" % e)
    finally:
        db.close() # this has no effect if the db is not opened

The :class:`sophia.Database` object only deals with bytes (named :class:`str` under Python 2, :class:`bytes` in Python 3). Transparent data serialization is done by the :class:`sophia.ObjectDatabase` class, for which see below.
       

Storing and deleting records
============================

Records are added with the :meth:`Database.set()` method, and deleted with :meth:`Database.delete()`::

    # create a database
    db = sophia.Database()
    db.open("actresses")
    
    # add some records
    db.set("Audrey Hepburn", "Breakfast at Tiffany's")
    db.set("Grace Kelly", "To Catch a Thief")
    
    # update a record
    db.set("Audrey Hepburn", "War and Peace")
    
    # delete a record
    db.delete("Audrey Hepburn")

When using these functions, database modifications are performed atomically, which adds some overhead. If you want to store several records at once, you can use explicit transactions. The added items will then be kept into memory until the transaction is committed or aborted, at which time they will be saved or left away, respectively. Transactions are really easy to perform::

    # start a transaction
    db.begin()
    
    # add some items, remove some others
    db.set("Scarlett Johansson", "The Black Dahlia")
    db.set("Uma Thurman", "Pulp Fiction")
    db.delete("Grace Kelly")
    
    # save the changes
    db.commit()
    
    # make another transaction   
    db.begin()
    db.set("Nicole Kidman", "Shakespeare in Love")
    db.set("Gwyneth Paltrow", "Dogville")
    
    # oops, interverted the films names, so abort the transaction
    db.rollback()


Retrieving records
==================

Records can be retrieved by using the :meth:`Database.get()` method, and checked for existence with the :meth:`Database.contains()` method.

    >>> db.get("Scarlett Johansson")
    "The Black Dahlia"
    >>> db.contains("Scarlett Johansson")
    True
    >>> db.contains("Nicole Kidman")
    False

If a second argument is given to :meth:`Database.get()`, it will be returned as value if the key is not in the database. The default is to return `None` when a key is missing.

    >>> db.get("Gwyneth Paltrow")
    >>> db.get("Gwyneth Paltrow", "A perfect number")
    "A perfect number"


Traversing records
==================

Records can be traversed in order with the :meth:`Database.iterkeys()`, :meth:`Database.itervalues()`, or :meth:`Database.iteritems()` methods, which yield respectively the keys, the values, or the pairs of (key, value) in the database. These methods take two optional arguments: the key at which to start iterating (which need not necessarily exist in the database), and the order in which the records should be traversed. Possible values for the `order` argument are:

* :const:`sophia.SPGT`  - increasing order (skipping the key, if it is equal)
* :const:`sophia.SPGTE` - increasing order (with key)
* :const:`sophia.SPLT`  - decreasing order (skipping the key, if it is equal)
* :const:`sophia.SPLTE` - decreasing order

By default, iteration is done in lexicographical order, and starts at the very first key in the database, including it.

Here is, for example, how you would iterate over all the keys in a database starting with a given prefix, skipping the prefix itself (if it exists), and in lexicographical order::

    import sophia, itertools
    
    def iter_prefixes(db, prefix):
        cursor = db.iterkeys(prefix, sophia.SPGT)
        return itertools.takewhile(lambda key: key.startswith(prefix), cursor)
    
    # create a database with some records to check this works
    db = sophia.Database()
    db.open("prefix_db")
    db.set("think", "")
    db.set("thought", "")
    db.set("thinking", "")
    db.set("thinker", "")

At the prompt::

    >>> list(iter_prefixes(db, "think"))
    ['thinker', 'thinking']


Storing rich objects
====================

It is possible to store any kind of Python object in a database, as long as this object is serializable. The class :class:`sophia.ObjectDatabase` defines an interface for marshalling/unmarshalling data transparently. By default, it serialises objects (both keys and values) with the :mod:`pickle` module. If the shape of your data permits it, you may prefer to use the :mod:`struct` module. It is faster than :mod:`pickle`, and is language-independent (which means you can open the same database from C, Python, Lua, or what not, without pain), but on the other hand can only handle fixed-type data.

Here is, for example, how you would write an interface for a database intended to be used for storing mappings of unicode keys to unsigned integers. Here we choose to encode the keys in UTF-8, and to represent the integers as C :c:type:`unsigned long`, packed in network order (so that the database is portable across architectures)::

    import sophia, struct
	
    # our custom structure for packing integers
    value_struct = struct.Struct("!L")
	
    # serialization functions
    
    pack_key     = lambda k: k.encode("utf-8")
    unpack_key   = lambda k: k.decode("utf-8")
    pack_value   = value_struct.pack
    unpack_value = lambda v: value_struct.unpack(v)[0]
    
    # anonymous function for instantiating the `ObjectDatabase` class
    # with our custom marshalling functions
    
    MyDB = lambda: sophia.ObjectDatabase(pack_key, unpack_key,
        pack_value, unpack_value)

You can now create a database and access it as expected:

    >>> db = MyDB()
    >>> db.open("my_db")
    >>> db.set(u"Penny", 22)
    >>> db.set(u"Bruce", 45)
    >>> db.get(u"Penny")
    22
    >>> list(db.iteritems())
    [(u'Bruce', 45), (u'Penny', 22)]


Tuning
======

All the `tuning options <http://sphia.org/sp_ctl.html>`_ available in the C API are accessible from Python, at the exception of :const:`SPALLOC` and :const:`SPDIR`. Options are set on the :class:`Database` object itself with the method :meth:`Database.setopt()`, which takes as argument the constant identifying the option (:const:`SPCMP`, :const:`SPPAGE`, etc.), and one or two arguments (depending on the option) indicating the value(s) to be set. The relevant constants are exported into the python module, so you can access them as :const:`sophia.SPCMP`, :const:`sophia.SPPAGE`, etc.

The more useful option is perhaps :const:`SPCMP`, which can be used to define a custom function for ordering the keys while traversing the database. This function will be passed as argument the first key, its length, the second one, and the corresponding length, in that order, and should return -1, 0, or 1, respectively, if the first key is lower, equal, or higher than the second one. Here is how you would define one for comparing keys on their length, and attach it to your database instance::

    def compare_on_length(key1, len1, key2, len2):
        return -1 if len1 < len2 else int(len1 > len2)

    db = sophia.Database()
    db.setopt(sophia.SPCMP, compare_on_length)

    # add some records to check this works
    db.open("cmp_db")
    db.set("long key", "")
    db.set("key", "")
    db.set("very long key", "")

At the prompt::

    >>> list(db.iterkeys())
    ['key', 'long key', 'very long key']

Options persist into a :class:`Database` object until it is destroyed, and can't be changed while the database is opened.

On threading
============

Two things should be kept in mind if you intend to use sophia in a threaded environment:

* It is not possible to open more than one connection to the same database at the same time. On the other hand, it is ok to share the same database object between threads.
* It is not possible to perform a transaction or to set/delete a record while a :class:`sophia.Cursor` object (as returned by the group of methods :meth:`Database.iterkeys()`, etc.) is alive. It is, however, possible to create a cursor object while a transaction is active.

A class :class:`sophia.ThreadedDatabase` handles the second case by protecting the necessary functions with a lock. It should not be used, however, when it isn't necessary, as it imposes a significant overhead on writing operations. Here is a summary of what classes you should use depending on what you intend to do with them:

* If you don't work in a threaded environment, use the :class:`sophia.Database` and :class:`sophia.ObjectDatabase` classes.
* If you work in a threaded environment BUT don't need to iterate over the database, do the same as above, and make sure you create and open the database object in the main thread, before passing it around to the other threads.
* If you work in a threaded environement AND need to iterate over the database, use the :class:`sophia.ThreadedDatabase` class and its sibling :class:`sophia.ThreadedObjectDatabase`.

Cursor pitfall
==============

A special behaviour has to be kept in mind when dealing with cursors: it is not possible to close or reopen a database while a cursor is in use. The return value of :meth:`Database.close()` and :meth:`Database.open()`, in addition with :meth:`Database.is_closed()`, will tell you whether the database has been effectively closed or re-opened. If any of these returns `False`, you should understand that there is a cursor lying out there. The database will effectively be closed as soon as the last remaining opened cursor is closed. A cursor is closed either when it has been exhausted through iteration, or when it goes out of scope::

    >>> # open a database and create a cursor
    >>> db.open("pitfall_db")
    True
    >>> cursor = db.iterkeys()
    >>> # try to close the database while a cursor is active; this doesn't work
    >>> db.close()
    False
    >>> # delete the cursor to make it work; the database will be closed immediately after
    >>> del cursor
    >>> db.is_closed()
    True

Final notes
===========

You may want to check the :doc:`reference` for a summary, and the `sophia documentation <http://sphia.org/documentation.html>`_ for more details.
