# PAGE CACHE TESTS

Clean page cache

echo 3 > /proc/sys/vm/drop_caches

Fill page cache

./pagecachefill

See how much of the file is in the page cache

./getpagecache testfile.bin

Prink the number of node in page cache xarray

stat testfile.bin
