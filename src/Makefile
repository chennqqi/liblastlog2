liblastlog2.so: lastlog2.c
	$(CC) -ggdb3 -Wall -Wextra -fPIC --shared $^ -o $@ -Wl,-soname='liblastlog2.so'

lastlog:
	$(CC) -o lastlog -Wall -Wextra backend.c bck-lastlog2.c lastlog.c
all: liblastlog2.so
