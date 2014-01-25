#ifdef __cplusplus
extern "C" {
#endif

#ifdef PSP_DEBUG
	#undef NDEBUG
#endif

#include <sophia.h>
#include <Python.h>

#if PY_MAJOR_VERSION < 3
    #define PyBytes_FromStringAndSize PyString_FromStringAndSize
    #define PyBytes_AsStringAndSize   PyString_AsStringAndSize
#endif

typedef struct {
    PyObject_HEAD
    void *db;              /* pointer to the sophia database object */
    void *env;             /* pointer to the sophia environment object */
    size_t cursors;        /* number of cursors currently in use */
    char close_me;         /* 1 if the database should be closed after the last
                            * alive cursor is destroyed, 0 otherwise */
    PyObject *cmp_fun;     /* pointer to the python custom comparison function */
} SophiaDB;

typedef struct {
    PyObject_HEAD
    SophiaDB *db;          /* pointer to the database attached to this cursor */
    void *cursor;          /* pointer to the sophia cursor object */
} SophiaCursor;

static PyObject *SophiaError;

static PyObject * sophia_db_new(PyTypeObject *, PyObject *, PyObject *);
static void sophia_db_dealloc(SophiaDB *);
static int sophia_db_init(SophiaDB *);
static PyObject * sophia_db_set_option(SophiaDB *, PyObject *);
static PyObject * sophia_db_open(SophiaDB *, PyObject *);
static PyObject * sophia_db_close(SophiaDB *);
static PyObject * sophia_db_is_closed(SophiaDB *db);
static PyObject * sophia_db_set(SophiaDB *, PyObject *);
static PyObject * sophia_db_get(SophiaDB *, PyObject *);
static PyObject * sophia_db_contains(SophiaDB *, PyObject *);
static PyObject * sophia_db_delete(SophiaDB *, PyObject *);
static PyObject * sophia_db_count_records(SophiaDB *db);
static PyObject * sophia_db_begin(SophiaDB *);
static PyObject * sophia_db_commit(SophiaDB *);
static PyObject * sophia_db_rollback(SophiaDB *);
static PyObject * sophia_db_iter_keys(SophiaDB *, PyObject *, PyObject *);
static PyObject * sophia_db_iter_values(SophiaDB *, PyObject *, PyObject *);
static PyObject * sophia_db_iter_items(SophiaDB *, PyObject *, PyObject *);

static PyObject * sophia_cursor_new(SophiaDB *, PyTypeObject *, PyObject *, PyObject *);
static void sophia_cursor_dealloc(SophiaCursor *);
static PyObject * sophia_cursor_next_key(SophiaCursor *cursor);
static PyObject * sophia_cursor_next_value(SophiaCursor *cursor);
static PyObject * sophia_cursor_next_item(SophiaCursor *cursor);

static int sophia_db_close_internal(SophiaDB *db);
static inline void sophia_cursor_dealloc_internal(SophiaCursor *cursor);
static int pylong_to_uint32_t(PyObject *, uint32_t *);
static int pyfloat_to_double(PyObject *, double *);
static PyObject * sophia_db_set_cmp_fun(SophiaDB *, PyObject *);
static inline int sophia_compare_default(char *, size_t, char *, size_t, void *);
static int sophia_compare_custom(char *, size_t, char *, size_t, void *);

static PyMethodDef sophia_db_methods[] = {
    {"__init__", (PyCFunction)sophia_db_init, METH_NOARGS, NULL},
    {"len", (PyCFunction)sophia_db_count_records, METH_NOARGS, NULL},
    {"setopt", (PyCFunction)sophia_db_set_option, METH_VARARGS, NULL},
    {"open", (PyCFunction)sophia_db_open, METH_VARARGS, NULL},
    {"close", (PyCFunction)sophia_db_close, METH_NOARGS, NULL},
    {"is_closed", (PyCFunction)sophia_db_is_closed, METH_NOARGS, NULL},
    {"get", (PyCFunction)sophia_db_get, METH_VARARGS, NULL},
    {"set", (PyCFunction)sophia_db_set, METH_VARARGS, NULL},
    {"delete", (PyCFunction)sophia_db_delete, METH_VARARGS, NULL},
    {"contains", (PyCFunction)sophia_db_contains, METH_VARARGS, NULL},
    {"begin", (PyCFunction)sophia_db_begin, METH_NOARGS, NULL},
    {"commit", (PyCFunction)sophia_db_commit, METH_NOARGS, NULL},
    {"rollback", (PyCFunction)sophia_db_rollback, METH_NOARGS, NULL},
    {"iterkeys", (PyCFunction)sophia_db_iter_keys, METH_VARARGS | METH_KEYWORDS, NULL},
    {"itervalues", (PyCFunction)sophia_db_iter_values, METH_VARARGS | METH_KEYWORDS, NULL},
    {"iteritems", (PyCFunction)sophia_db_iter_items, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL},
};

static PyTypeObject SophiaDBType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "sophia.Database",             /* tp_name */
    sizeof(SophiaDB),              /* tp_basicsize */
    0,                             /* tp_itemsize */
    (destructor)sophia_db_dealloc, /* tp_dealloc */
    0,                             /* tp_print */
    0,                             /* tp_getattr */
    0,                             /* tp_setattr */
    0,                             /* tp_reserved */
    0,                             /* tp_repr */
    0,                             /* tp_as_number */
    0,                             /* tp_as_sequence */
    0,                             /* tp_as_mapping */
    0,                             /* tp_hash  */
    0,                             /* tp_call */
    0,                             /* tp_str */
    0,                             /* tp_getattro */
    0,                             /* tp_setattro */
    0,                             /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |           /* tp_flags */
    Py_TPFLAGS_BASETYPE,
    0,                             /* tp_doc */
    0,                             /* tp_traverse */
    0,                             /* tp_clear */
    0,                             /* tp_richcompare */
    0,                             /* tp_weaklistoffset */
    0,                             /* tp_iter */
    0,                             /* tp_iternext */
    sophia_db_methods,             /* tp_methods */
    0,                             /* tp_members */
    0,                             /* tp_getset */
    0,                             /* tp_base */
    0,                             /* tp_dict */
    0,                             /* tp_descr_get */
    0,                             /* tp_descr_set */
    0,                             /* tp_dictoffset */
    (initproc)sophia_db_init,      /* tp_init */
    0,                             /* tp_alloc */
    sophia_db_new,                 /* tp_new */
};

static PyTypeObject SophiaCursorKeysType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "sophia.Cursor",                             /* tp_name */
    sizeof(SophiaCursor),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)sophia_cursor_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)sophia_cursor_next_key,       /* tp_iternext */
    0,                                          /* tp_methods */
    0,
};

static PyTypeObject SophiaCursorValuesType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "sophia.Cursor",                             /* tp_name */
    sizeof(SophiaCursor),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)sophia_cursor_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)sophia_cursor_next_value,     /* tp_iternext */
    0,                                          /* tp_methods */
    0,
};

static PyTypeObject SophiaCursorItemsType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "sophia.Cursor",                             /* tp_name */
    sizeof(SophiaCursor),                       /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)sophia_cursor_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)sophia_cursor_next_item,      /* tp_iternext */
    0,                                          /* tp_methods */
    0,
};

static PyObject *
sophia_db_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    SophiaDB *db = (SophiaDB *)type->tp_alloc(type, 0);
    if (!db)
        return NULL;
    db->db = NULL;
    db->env = sp_env();
    if (!db->env)
        return PyErr_NoMemory();
    db->cmp_fun = NULL;
    return (PyObject *)db;
}

static void
sophia_db_dealloc(SophiaDB *db)
{
    if (db->db)
        sophia_db_close_internal(db);
    sp_destroy(db->env);
    if (db->cmp_fun)
        Py_DECREF(db->cmp_fun);
    Py_TYPE(db)->tp_free((PyObject *)db);
}

static int
sophia_db_init(SophiaDB *db)
{
    if (db->db && sophia_db_close_internal(db) == -1)
        return -1;
    return 0;
}

/* Open (or reopen) the underlying sophia database iff no cursors
 * are still in use.
 */
static PyObject *
sophia_db_open(SophiaDB *db, PyObject *args)
{
    char *path;
    
    if (!PyArg_ParseTuple(args, "s:__init__", &path))
        return NULL;
    
    if (db->db) {
        int status = sophia_db_close_internal(db);
        if (status == 0) {
            db->close_me = 1;
            Py_RETURN_FALSE;
        }
        else if (status == -1)
            return NULL;
    }
    
    if (sp_ctl(db->env, SPDIR, SPO_CREAT | SPO_RDWR, path) == -1 ||
        (!db->cmp_fun && sp_ctl(db->env, SPCMP, sophia_compare_default, NULL) == -1))
        return PyErr_NoMemory();

    db->db = sp_open(db->env);
    if (!db->db) {
        PyErr_SetString(SophiaError, sp_error(db->env));
        return NULL;
    }
    
    db->cursors = 0;
    db->close_me = 0;
    
    Py_RETURN_TRUE;
}

/* Destroy the underlying sophia database iff no cursors are still in use,
 * as it would be impossible to destroy them afterwards (trying to do so results
 * in a segfault). If a cursor is still in use, mark the python database as
 * "to be closed", and effectively close it when the last remaining cursor
 * has been destroyed. A call to `Database.open()` sets the marker "to be closed"
 * to false. 
 */
static int
sophia_db_close_internal(SophiaDB *db)
{
    if (!db->db)
        return 1;
    if (db->cursors > 0) {
        return 0;
    }
    if (sp_destroy(db->db) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->env));
        return -1;
    }
    db->db = NULL;
    return 1;
}

static PyObject *
sophia_db_close(SophiaDB *db)
{
    int rv = sophia_db_close_internal(db);
    if (rv == 1)
        Py_RETURN_TRUE;
    else if (rv == 0) {
        db->close_me = 1;
        Py_RETURN_FALSE;
    }
    return NULL;
}

static PyObject *
sophia_db_is_closed(SophiaDB *db)
{
    if (!db->db)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static int
pylong_to_uint32_t(PyObject *num, uint32_t *out)
{
    unsigned long value;
    
    value = PyLong_AsUnsignedLong(num);
    if (PyErr_Occurred())
        return -1;
    if (value > UINT32_MAX) {
        PyErr_SetString(PyExc_ValueError, "option value is out of bound");
        return -1;
    }
    
    *out = (uint32_t)value;
    return 0;
}

static int
pyfloat_to_double(PyObject *num, double *out)
{
    if (!PyFloat_Check(num)) {
        PyErr_SetString(PyExc_TypeError, "expected a float");
        return -1;
    }
    *out = PyFloat_AsDouble(num);
    if (PyErr_Occurred())
        return -1;
    return 0;
}

/* Attach a python comparison function to the database instance.
 * Passing `None` resets the comparison function to the original
 * default one.
 */
static PyObject *
sophia_db_set_cmp_fun(SophiaDB *db, PyObject *fun)
{
    int rv;
    
    if (fun == Py_None) {
        if (!db->cmp_fun)
            return 0;
        Py_DECREF(db->cmp_fun);
        db->cmp_fun = NULL;
        rv = sp_ctl(db->env, SPCMP, sophia_compare_default, NULL);
    }
    else if (PyCallable_Check(fun)) {
        if (db->cmp_fun)
            Py_DECREF(db->cmp_fun);
        Py_INCREF(fun);
        db->cmp_fun = fun;
        rv = sp_ctl(db->env, SPCMP, sophia_compare_custom, fun);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "expected either a callable or None");
        return NULL;
    }
    
    if (rv == -1) {
        PyErr_SetString(SophiaError, sp_error(db->env));
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sophia_db_set_option(SophiaDB *db, PyObject *args)
{
    int rv, option;
    PyObject *pvalue, *pvalue2 = NULL;
    
    if (!PyArg_ParseTuple(args, "iO|O:setopt", &option, &pvalue, &pvalue2))
        return NULL;
    
    if (option == SPCMP) {
        return sophia_db_set_cmp_fun(db, pvalue);
    }
    else if (option == SPPAGE || option == SPMERGEWM) {
    
        uint32_t value;
        if (pylong_to_uint32_t(pvalue, &value) == -1)
            return NULL;
        rv = sp_ctl(db->env, option, value);
    }
    else if (option == SPGC || option == SPMERGE) {
    
        int value = PyObject_IsTrue(pvalue);
        if (value == -1)
            return NULL;
        rv = sp_ctl(db->env, option, value);
    }
    else if (option == SPGCF) {
    
        double value;
        if (pyfloat_to_double(pvalue, &value) == -1)
            return NULL;
        rv = sp_ctl(db->env, option, value);
    }
    else if (option == SPGROW) {

        if (pvalue2 == NULL) {
            PyErr_SetString(PyExc_ValueError, "expected a third argument");
            return NULL;
        }
        
        uint32_t new_size;
        double resize;
        if (pylong_to_uint32_t(pvalue, &new_size) == -1 ||
            pyfloat_to_double(pvalue2, &resize) == -1)
            return NULL;
        rv = sp_ctl(db->env, option, new_size, resize);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "unknown option");
        return NULL;
    }
    
    if (rv == -1) {
        PyErr_SetString(SophiaError, sp_error(db->env));
        return NULL;
    }
    Py_RETURN_NONE;
}

#define ensure_is_opened(pdb, rv)                                     \
do {                                                                  \
    if (!(pdb)->db) {                                                  \
        PyErr_SetString(SophiaError, "operation on a closed database"); \
        return (rv);                                                    \
    }                                                                  \
} while (0)

static PyObject *
sophia_db_set(SophiaDB *db, PyObject *args)
{
    char *key, *value;
    PyObject *pkey, *pvalue;
    Py_ssize_t ksize, vsize;
    
    ensure_is_opened(db, NULL);
    
    if (!PyArg_UnpackTuple(args, "set", 2, 2, &pkey, &pvalue)
        || PyBytes_AsStringAndSize(pkey, &key, &ksize) == -1
        || PyBytes_AsStringAndSize(pvalue, &value, &vsize) == -1)
        return NULL;
    
    if (sp_set(db->db, key, (size_t)ksize, value, (size_t)vsize) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    
    Py_RETURN_NONE;
}

static PyObject *
sophia_db_get(SophiaDB *db, PyObject *args)
{
    char *key;
    PyObject *pkey, *pvalue = NULL;
    void *value;
    Py_ssize_t ksize;
    size_t vsize;
    
    ensure_is_opened(db, NULL);
    
    if (!PyArg_UnpackTuple(args, "get", 1, 2, &pkey, &pvalue)
        || PyBytes_AsStringAndSize(pkey, &key, &ksize) == -1)
        return NULL;
        
    int rv = sp_get(db->db, key, (size_t)ksize, &value, &vsize);
    switch (rv) {
        case 1:
            break;
        case 0:
            if (pvalue)
                return pvalue;
            Py_RETURN_NONE;
        default:
            PyErr_SetString(SophiaError, sp_error(db->db));
            return NULL;
    }
    
    pvalue = PyBytes_FromStringAndSize(value, (Py_ssize_t)vsize);
    free(value);
    return pvalue;
}

static PyObject *
sophia_db_contains(SophiaDB *db, PyObject *args)
{
    char *key;
    PyObject *pkey;
    Py_ssize_t ksize;
    
    ensure_is_opened(db, NULL);
    
    if (!PyArg_UnpackTuple(args, "get", 1, 1, &pkey)
        || PyBytes_AsStringAndSize(pkey, &key, &ksize) == -1)
        return NULL;
    
    int rv = sp_get(db->db, key, (size_t)ksize, NULL, NULL);
    switch (rv) {
        case 1:
            Py_RETURN_TRUE;
        case 0:
            Py_RETURN_FALSE;
        default:
            PyErr_SetString(SophiaError, sp_error(db->db));
            return NULL;
    }
}

static PyObject *
sophia_db_delete(SophiaDB *db, PyObject *args)
{
    char *key;
    PyObject *pkey;
    Py_ssize_t ksize;
    
    ensure_is_opened(db, NULL);
    
    if (!PyArg_UnpackTuple(args, "delete", 1, 1, &pkey)
        || PyBytes_AsStringAndSize(pkey, &key, &ksize) == -1)
        return NULL;
    
    if (sp_delete(db->db, key, (size_t)ksize) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    
    Py_RETURN_NONE;
}

/* Count the number of records in the database. This is O(n) time,
 * and then absolutely inefficient, but keeping an up-to-date counter
 * of the records would require to check the existence of a record before
 * each insert or delete operation, which would be very costly at the end.
 */
static PyObject *
sophia_db_count_records(SophiaDB *db)
{
    size_t count = 0;
    
    ensure_is_opened(db, NULL);
    
    void *cur = sp_cursor(db->db, SPGT, NULL, 0);
    if (!cur) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    
    while ((sp_fetch(cur)))
        count++;
    sp_destroy(cur);
    
    return PyLong_FromSize_t(count);
}

static PyObject *
sophia_db_begin(SophiaDB *db)
{
    ensure_is_opened(db, NULL);
    
    if (sp_begin(db->db) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sophia_db_commit(SophiaDB *db)
{
    ensure_is_opened(db, NULL);
    
    if (sp_commit(db->db) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sophia_db_rollback(SophiaDB *db)
{
    ensure_is_opened(db, NULL);
    
    if (sp_rollback(db->db) == -1) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
sophia_db_iter_keys(SophiaDB *db, PyObject *args, PyObject *kw)
{
    ensure_is_opened(db, NULL);
    return sophia_cursor_new(db, &SophiaCursorKeysType, args, kw);
}

static PyObject *
sophia_db_iter_values(SophiaDB *db, PyObject *args, PyObject *kw)
{
    ensure_is_opened(db, NULL);
    return sophia_cursor_new(db, &SophiaCursorValuesType, args, kw);
}

static PyObject *
sophia_db_iter_items(SophiaDB *db, PyObject *args, PyObject *kw)
{
    ensure_is_opened(db, NULL);
    return sophia_cursor_new(db, &SophiaCursorItemsType, args, kw);
}

static PyObject *
sophia_cursor_new(SophiaDB *db, PyTypeObject *cursortype,
                     PyObject *args, PyObject *kwargs)
{
    SophiaCursor *pcur;
    int order = SPGTE;
    char *begin = NULL;
    PyObject *pbegin = NULL;
    Py_ssize_t bsize = 0;
    
    static char *keywords[] = {"start_key", "order", NULL};
    
    ensure_is_opened(db, NULL);
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi", keywords, &pbegin, &order)
        || (pbegin && pbegin != Py_None && PyBytes_AsStringAndSize(pbegin, &begin, &bsize) == -1))
        return NULL;

    pcur = PyObject_New(SophiaCursor, cursortype);
    if (!pcur)
        return NULL;
    
    void *cursor = sp_cursor(db->db, order, begin, (size_t)bsize);
    if (!cursor) {
        PyErr_SetString(SophiaError, sp_error(db->db));
        return NULL;
    }
    
    Py_INCREF(db);
    db->cursors++;
    pcur->db = db;
    pcur->cursor = cursor;
    return (PyObject *)pcur;
}

/* Destroys the underlying sophia cursor, without deallocating the Python object
 * which encapsulates it. This function is called either when the sophia cursor
 * has traversed all the records requested, or when the object goes out of scope. 
 */
static inline void
sophia_cursor_dealloc_internal(SophiaCursor *cursor)
{
    assert(cursor->cursor);
    assert(cursor->db->cursors >= 1);
    
    /* close the cursor first, only then the database, if needed */
    sp_destroy(cursor->cursor);
    cursor->cursor = NULL;
    
    cursor->db->cursors--;
    if (cursor->db->close_me && cursor->db->cursors == 0) {
        sophia_db_close_internal(cursor->db);
        cursor->db->close_me = 0;
    }
    Py_DECREF(cursor->db);
    cursor->db = NULL;
}

/* Destroys the Python wrapped cursor, and the underlying cursor if needed */
static void
sophia_cursor_dealloc(SophiaCursor *cursor)
{
    if (cursor->cursor)
        sophia_cursor_dealloc_internal(cursor);
    PyObject_Del(cursor);
}

/* Should we stop iterating the database with this cursor? If so, destroy
 * the sophia cursor object, but leave intact its Python wrapper.
 */
static inline int
sophia_stop_iteration(SophiaCursor *cursor)
{
    if (!cursor->cursor || !sp_fetch(cursor->cursor)) {
        if (cursor->cursor)
            sophia_cursor_dealloc_internal(cursor);
        return 1;
    }
    return 0;
}

static PyObject *
sophia_cursor_next_key(SophiaCursor *cursor)
{
    if (sophia_stop_iteration(cursor))
        return NULL;
    
    const char *key = sp_key(cursor->cursor);
    size_t ksize = sp_keysize(cursor->cursor);

    if (key == NULL || ksize == 0) {
        PyErr_SetString(SophiaError, "cursor failed");
        return NULL;
    }
    
    return PyBytes_FromStringAndSize(key, (Py_ssize_t)ksize);
}

static PyObject *
sophia_cursor_next_value(SophiaCursor *cursor)
{
    if (sophia_stop_iteration(cursor))
        return NULL;
    
    const char *value = sp_value(cursor->cursor);
    size_t vsize = sp_valuesize(cursor->cursor);
    
    if (value == NULL || vsize == 0) {
        PyErr_SetString(SophiaError, "cursor failed");
        return NULL;
    }
    
    return PyBytes_FromStringAndSize(value, (Py_ssize_t)vsize);
}

static PyObject *
sophia_cursor_next_item(SophiaCursor *cursor)
{
    size_t ksize, vsize;
    PyObject *rv, *pkey, *pvalue;
    
    if (sophia_stop_iteration(cursor))
        return NULL;
    
    const char *key = sp_key(cursor->cursor);
    ksize = sp_keysize(cursor->cursor);
    const char *value = sp_value(cursor->cursor);
    vsize = sp_valuesize(cursor->cursor);
    
    if (key == NULL || value == NULL || ksize == 0 || vsize == 0) {
        PyErr_SetString(SophiaError, "cursor failed");
        return NULL;
    }
    
    pkey = PyBytes_FromStringAndSize(key, (Py_ssize_t)ksize);
    pvalue = PyBytes_FromStringAndSize(value, (Py_ssize_t)vsize);
    rv = PyTuple_Pack(2, pkey, pvalue);
    
    Py_DECREF(pkey);
    Py_DECREF(pvalue);
    
    return rv;
}

static inline int
sophia_compare_default(char *a, size_t asz, char *b, size_t bsz, void *arg)
{
    int cmp = memcmp(a, b, (asz < bsz ? asz : bsz));
    return (cmp == 0 ?
        (asz == bsz ? 0 : (asz < bsz ? -1 : 1)) :
        (cmp < 0 ? -1 : 1)
    );
}

static int
sophia_compare_custom(char *a, size_t asz, char *b, size_t bsz, void *cmp_fun)
{
    PyObject *pasz = NULL, *pbsz = NULL, *pa = NULL, *pb = NULL, *prv = NULL;
    
    if (  !(pasz = PyLong_FromSize_t(asz))
        || !(pbsz = PyLong_FromSize_t(bsz))
        || !(pa = PyBytes_FromStringAndSize(a, (Py_ssize_t)asz))
        || !(pb = PyBytes_FromStringAndSize(b, (Py_ssize_t)bsz)))
        goto error_args;
    
    prv = PyObject_CallFunctionObjArgs((PyObject *)cmp_fun,
        pa, pasz, pb, pbsz, NULL);
    
    Py_DECREF(pasz);
    Py_DECREF(pbsz);
    Py_DECREF(pa);
    Py_DECREF(pb);
    
    if (prv == NULL) {
        PyErr_SetString(SophiaError, "failed to call custom comparison function");
        goto error_call;
    }
    
    long rv = PyLong_AsLong(prv);
    
    if (PyErr_Occurred()) {
        PyErr_SetString(SophiaError, "custom comparison function returned garbage");
        goto error_call;
    }
    
    Py_DECREF(prv);
    
    return (rv < 0 ? -1 : (rv > 0 ? 1 : 0));

/* Fall back to using the default comparison function */
error_args:
    Py_XDECREF(pasz);
    Py_XDECREF(pbsz);
    Py_XDECREF(pa);
    Py_XDECREF(pb);
    return sophia_compare_default(a, asz, b, bsz, NULL);
error_call:
    Py_XDECREF(prv);
    return sophia_compare_default(a, asz, b, bsz, NULL);
}

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef _sophiamodule = {
   PyModuleDef_HEAD_INIT, "_sophia", "docstring", -1, NULL
};

#define PSP_NOTHING NULL
PyMODINIT_FUNC
PyInit__sophia(void) 

#else

#define PSP_NOTHING
PyMODINIT_FUNC
init_sophia(void)

#endif
{
    static char *sophia_constant_names[] = {"SPGT", "SPGTE", "SPLT", "SPLTE",
        "SPCMP", "SPPAGE", "SPMERGEWM", "SPGC", "SPMERGE", "SPGCF", "SPGROW", NULL};
    
    static int sophia_constant_values[] = {SPGT, SPGTE, SPLT, SPLTE,
        SPCMP, SPPAGE, SPMERGEWM, SPGC, SPMERGE, SPGCF, SPGROW};
    
    if (PyType_Ready(&SophiaDBType) == -1)
        return PSP_NOTHING;
    SophiaError = PyErr_NewException("sophia.Error", NULL, NULL);
    if (!SophiaError)
        return PSP_NOTHING;

#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&_sophiamodule);
#else
    PyObject *module = Py_InitModule("_sophia", NULL);
#endif
    if (!module)
        return PSP_NOTHING;
    
    char **names = sophia_constant_names;
    int *values = sophia_constant_values;
    while (*names) {
        if (PyModule_AddIntConstant(module, *names++, *values++) == -1)
            return PSP_NOTHING;
    }
    
    Py_INCREF(&SophiaDBType);
    Py_INCREF(SophiaError);
    
    if (PyModule_AddObject(module, "Database", (PyObject *)&SophiaDBType) == -1 ||
        PyModule_AddObject(module, "Error", SophiaError) == -1)
        return PSP_NOTHING;

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

#ifdef __cplusplus
}
#endif
