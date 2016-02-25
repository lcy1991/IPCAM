ARCH:=$(shell uname -m)
BIT32:=i686
BIT64:=x86_64
OBJ=clear.o \
	compare.o \
	copy.o \
	gen_uuid.o \
	isnull.o \
	pack.o \
	parse.o \
	unpack.o \
	unparse.o \
	uuid_time.o \
	randutils.o \


CC=gcc
CFLAGS=-Wno-multichar

ifeq ($(ARCH),$(BIT64))
#FLAG64=-fPIC
endif

INCLUDES= -I./
#LDLIBS= -lpthread
#uidtest:uuidtest.c libuuid.a
#	$(CC)  $(FLAG64) $(CFLAGS) $(INCLUDES) uuidtest.c libuuid.a -luuid -o uuidtest
#libuuid.so:$(OBJ)
#	$(CC) -shared  $(FLAG64) $(LDLIBS) $(OBJ) -o libuuid.so 

libuuid.a:$(OBJ)
	ar  crv libuuid.a  $(FLAG64)  $(OBJ) 
	mv libuuid.a ../libuuid.a
	
$(OBJ): %.o: %.c
	$(CC) -c $(FLAG64) $(CFLAGS)  $< -o $@
.PHONY:clean
clean:
	rm -f *.o ../libuuid.a
