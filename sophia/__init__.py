#!/usr/bin/env python

__all__ = ['Database', 'Error', 'ObjectDatabase', 'SPCMP', 'SPGC', 'SPGCF', 'SPGROW', 'SPGT', 'SPGTE', 'SPLT', 'SPLTE', 'SPMERGE', 'SPMERGEWM', 'SPPAGE', 'ThreadedDatabase', 'ThreadedObjectDatabase']

from _sophia import *
import threading, pickle


class ObjectDatabase(Database):

    """Database model for storing arbitrary kinds of objects.
    
    `pack_key`, `unpack_key`, `pack_value`, and `unpack_value`, should be callables that, when passed
    an object as parameter, return a byte representation of it, suitable for storage. By default, all
    these functions use the :mod:`pickle` module.
    """
    
    def __init__(self, pack_key=pickle.dumps, unpack_key=pickle.loads,
        pack_value=pickle.dumps, unpack_value=pickle.loads):
        self.pack_key = pack_key
        self.unpack_key = unpack_key
        self.pack_value = pack_value
        self.unpack_value = unpack_value
        super(ObjectDatabase, self).__init__()
        

    def get(self, key, default=None):
        value = super(ObjectDatabase, self).get(self.pack_key(key), default)
        return default if value is default else self.unpack_value(value)
    
    def set(self, key, value):
        return super(ObjectDatabase, self).set(self.pack_key(key), self.pack_value(value))
    
    def delete(self, key):
        return super(ObjectDatabase, self).delete(self.pack_key(key))

    def iterkeys(self, start_key=None, order=SPGTE):
        begin = start_key if start_key is None else self.pack_key(start_key)
        return (self.unpack_key(k) for k in super(ObjectDatabase, self).iterkeys(begin, order))
    
    def itervalues(self, start_key=None, order=SPGTE):
        begin = start_key if start_key is None else self.pack_key(start_key)
        return (self.unpack_value(v) for v in super(ObjectDatabase, self).itervalues(begin, order))
    
    def iteritems(self, start_key=None, order=SPGTE):
        begin = start_key if start_key is None else self.pack_key(start_key)
        return ((self.unpack_key(k), self.unpack_value(v)) \
            for k, v in super(ObjectDatabase, self).iteritems(begin, order))


class ThreadedDatabase(Database):

    """Thread-safe database model.
    
    It should only be used if you want to use a database in a threaded environment
    AND need to iterate over it. Otherwise, the vanilla :class:`Database` class is suitable
    (and more efficient).
    """

    def __init__(self):
        self._lock = threading.Lock()
        super(ThreadedDatabase, self).__init__()
    
    def _protect(self, method, *args, **kwargs):
        self._lock.acquire()
        try:
            method(*args, **kwargs)
        finally:
            self._lock.release()
    
    def _protect_iter(self, method, *args, **kwargs):
        self._lock.acquire()
        try:
            for something in method(*args, **kwargs):
                yield something
        finally:
            self._lock.release()
    
    def set(self, *args):
        return self._protect(super(ThreadedDatabase, self).set, *args)
    
    def delete(self, *args):
        return self._protect(super(ThreadedDatabase, self).delete, *args)

    def iterkeys(self, **kwargs):
        return self._protect_iter(super(ThreadedDatabase, self).iterkeys, **kwargs)
    
    def itervalues(self, **kwargs):
        return self._protect_iter(super(ThreadedDatabase, self).itervalues, **kwargs)
    
    def iteritems(self, **kwargs):
        return self._protect_iter(super(ThreadedDatabase, self).iteritems, **kwargs)


class ThreadedObjectDatabase(ObjectDatabase, ThreadedDatabase):
    
    """Mixing of a :class:`ThreadedDatabase` and an :class:`ObjectDatabase`."""
    
    pass
