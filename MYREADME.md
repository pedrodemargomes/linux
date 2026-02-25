# PAGE CACHE TESTS

Clean page cache

echo 3 > /proc/sys/vm/drop_caches

Fill page cache

./pagecachefill

If you fill the cache accessing the file in a random way the xarray will have more nodes than if you access the file sequentialy

See how much of the file is in the page cache

./getpagecache testfile.bin

Prink the number of node in page cache xarray

stat testfile.bin
