include config.mk

DEP = ebb.h ebb_request_parser.h rbtree.h
SRC = ebb.c ebb_request_parser.c rbtree.c
OBJ = ${SRC:.c=.o}

VERSION = 0.1
NAME=libebb
OUTPUT_LIB=$(NAME).$(SUFFIX).$(VERSION)
OUTPUT_A=$(NAME).a

LINKER=$(CC) $(LDOPT)

all: $(OUTPUT_A)

$(OUTPUT_LIB): $(OBJ) 
	@echo LINK $@
	$(LINKER) -o $(OUTPUT_LIB) $(OBJ) $(SONAME) $(LIBS)

$(OUTPUT_A): $(OBJ)
	@echo AR $@
	$(AR) cru $(OUTPUT_A) $(OBJ)
	@echo RANLIB $@
	$(RANLIB) $(OUTPUT_A)

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: ${DEP}

ebb_request_parser.c: ebb_request_parser.rl
	@echo RAGEL $<
	ragel -s -G2 $< -o $@

test: test_request_parser test_rbtree
	time ./test_request_parser
	./test_rbtree

test_rbtree: test_rbtree.o $(OUTPUT_A)
	@echo BUILDING test_rbtree
	@$(CC) $(CFLAGS) -o $@ $< $(OUTPUT_A)

test_request_parser: test_request_parser.o $(OUTPUT_A)
	@echo BUILDING test_request_parser
	@$(CC) $(CFLAGS) -o $@ $< $(OUTPUT_A)

examples: examples/hello_world

examples/hello_world: examples/hello_world.c $(OUTPUT_A) 
	@echo BUILDING examples/hello_world
	@$(CC) -I. $(LIBS) $(CFLAGS) -lev -o $@ $^

clean:
	@echo CLEANING
	@rm -f ${OBJ} $(OUTPUT_A) $(OUTPUT_LIB) libebb-${VERSION}.tar.gz 
	@rm -f test_rbtree test_request_parser 
	@rm -f examples/hello_world examples/hello_world.o

clobber: clean
	@echo CLOBBERING
	@rm -f ebb_request_parser.c

dist: clean $(SRC)
	@echo CREATING dist tarball
	@mkdir -p ${NAME}-${VERSION}
	@cp -R doc examples LICENSE Makefile README config.mk \
		ebb_request_parser.rl ${SRC} ${DEP} ${NAME}-${VERSION}
	@tar -cf ${NAME}-${VERSION}.tar ${NAME}-${VERSION}
	@gzip ${NAME}-${VERSION}.tar
	@rm -rf ${NAME}-${VERSION}

install: $(OUTPUT_LIB) $(OUTPUT_A)
	@echo INSTALLING ${OUTPUT_A} and ${OUTPUT_LIB} to ${PREFIX}/lib
	install -d -m755 ${PREFIX}/lib
	install -d -m755 ${PREFIX}/include
	install -m644 ${OUTPUT_A} ${PREFIX}/lib
	install -m755 ${OUTPUT_LIB} ${PREFIX}/lib
	ln -sfn $(PREFIX)/lib/$(OUTPUT_LIB) $(PREFIX)/lib/$(NAME).so
	@echo INSTALLING headers to ${PREFIX}/include
	install -m644 ebb.h ebb_request_parser.h ${PREFIX}/include 

uninstall:
	@echo REMOVING so from ${PREFIX}/lib
	rm -f ${PREFIX}/lib/${NAME}.*
	@echo REMOVING headers from ${PREFIX}/include
	rm -f ${PREFIX}/include/ebb.h
	rm -f ${PREFIX}/include/ebb_request_parser.h

upload_website:
	scp -r doc/index.html doc/icon.png rydahl@tinyclouds.org:~/web/public/libebb

.PHONY: all options clean clobber dist install uninstall test examples upload_website
