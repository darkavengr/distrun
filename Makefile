CC=gcc
OBJ=obj
OBJECT_FILES=helperfunctions.o getconfig.o distrun.o submitcommand.o ssh.o lookup.o
OUTFILE=distrun
CCFLAGS=-c -w
LDFLAGS=-lssh -DLIBSSH_STATIC

ifeq ($(OS),Windows_NT)
	OUTFILE += ".exe"
endif

distrun: $(OBJECT_FILES)
	$(CC) $(OBJECT_FILES) $(LDFLAGS) -o $(OUTFILE)

helperfunctions.o:
	$(CC) $(CCFLAGS) helperfunctions.c

getconfig.o:
	$(CC) $(CCFLAGS) getconfig.c

distrun.o:
	$(CC) $(CCFLAGS) distrun.c

submitcommand.o:
	$(CC) $(CCFLAGS) submitcommand.c

ssh.o:
	$(CC) $(CCFLAGS) ssh.c

lookup.o:
	$(CC) $(CCFLAGS) lookup.c

clean:
	rm $(OUTFILE) *.o



