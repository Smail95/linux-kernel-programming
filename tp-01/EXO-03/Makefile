.PHONY : all clean check
.ONESHELL:

CHECK_PATCH=./checkpatch.pl --no-tree

all : testHistory

%.o : %.c
	gcc -Wall -g -c $<

testHistory : testHistory.o commit.o history.o version.o
	gcc -Wall $^ -g -o $@

check : 
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean :
	rm -rf *.o testHistory