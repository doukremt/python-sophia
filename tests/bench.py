import sophia, shutil, sys, tempfile
import timeit, random

if sys.version_info.major < 3:
    def get_rand_k():
        return str(random.randint(0, 10000000))
else:
    def get_rand_k():
        return str(random.randint(0, 10000000)).encode()

def get_rand_p():
    return get_rand_k(), get_rand_k()

def sophia_write_batch(path, n):
    db = sophia.Database()
    db.open(path)
    db.begin()
    for i in range(n):
        db.set(*get_rand_p())
    db.commit()
    db.close()

def sophia_write_single(path, n):
    db = sophia.Database()
    db.open(path)
    for i in range(n):
        db.set(*get_rand_p())
    db.close()

def sophia_threaded_write_single(path, n):
    db = sophia.ThreadedDatabase()
    db.open(path)
    for i in range(n):
        db.set(*get_rand_p())
    db.close()

def sophia_search(path, n):
    db = sophia.Database()
    db.open(path)
    for i in range(n):
        db.get(get_rand_k())
    db.close()

def sophia_iterate(path):
    db = sophia.Database()
    db.open(path)
    for pair in db.iteritems():
        pass
    db.close()

template = """\
Results for %d records:
* random batch write: %fs
* random atomic write: %fs
* random threaded atomic write: %fs
* random read: %fs
* iteration over the whole database: %fs"""

def main():
    if len(sys.argv) < 2 or not sys.argv[1].isdigit():
        sys.stderr.write("Usage: %s NUMBER_OF_RECORDS\n" % sys.argv[0])
        sys.exit(1)
    n = int(sys.argv[1])
    sp_path = tempfile.mkdtemp()
    
    sp_pwrite = timeit.timeit(lambda: sophia_write_batch(sp_path, n), number=1)
    shutil.rmtree(sp_path)
    sp_swrite = timeit.timeit(lambda: sophia_write_single(sp_path, n), number=1)
    shutil.rmtree(sp_path)
    sp_tswrite = timeit.timeit(lambda: sophia_threaded_write_single(sp_path, n), number=1)
    sp_read = timeit.timeit(lambda: sophia_search(sp_path, n), number=1)
    sp_iterate = timeit.timeit(lambda: sophia_iterate(sp_path), number=1)
    shutil.rmtree(sp_path)
    print(template % (n, sp_pwrite, sp_swrite, sp_tswrite, sp_read, sp_iterate))

if __name__ == "__main__":
    main()
