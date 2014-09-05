all: liblastlog2.so
liblastlog2.so: backend.c bck-lastlog2.c
	$(CC) -ggdb3 -Wall -Wextra -fPIC --shared $^ -o $@ -Wl,-soname='liblastlog2.so'


