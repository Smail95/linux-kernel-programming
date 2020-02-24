.PHONY : all clean check
.ONESHELL:

CHECK_PATCH=./checkpatch.pl --no-tree

all : testVersion testOffset 

%.o : %.c
	gcc -Wall -c $<

testVersion : testVersion.o version.o
	gcc -Wall $^ -g -o $@

testOffset : testOffset.c version.o
	gcc -Wall $^ -g -o $@

check : 
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean :
	rm -rf *.o testVersion testOffset