CC=gcc
CFLAGS=-Wall -Wextra -lncurses
EXEC_CLIENT=client
EXEC_SERVER=server

TEST=test

SRCDIR=src
OBJDIR=obj

SRCOBJDIRCLIENT=$(OBJDIR)/src_client
SRCOBJDIRSERVER=$(OBJDIR)/src_server

TESTDIR=tests
TESTOBJDIR=$(OBJDIR)/$(TESTDIR)

CINTA=cinta
CINTAOBJ=$(OBJDIR)/$(CINTA)

VALGRIND=valgrind
VALGRIND_OPTS=--leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 -s


SRCFILESCLIENT := $(shell find $(SRCDIR) -type f -name "*.c" ! -name "server.c" ! -name "network_server.c" ! -name "communication_server.c")
SRCFILESSERVER := $(shell find $(SRCDIR) -type f -name "*.c" ! -name "client.c" ! -name "network_client.c" ! -name "communication_client.c")
TESTFILES := $(shell find $(TESTDIR) -type f -name "*.c")
CINTAFILES := $(shell find $(CINTA) -type f -name "*.c")

OBJFILESCLIENT := $(patsubst $(SRCDIR)/%.c,$(SRCOBJDIRCLIENT)/%.o,$(SRCFILESCLIENT))
OBJFILESSERVER := $(patsubst $(SRCDIR)/%.c,$(SRCOBJDIRSERVER)/%.o,$(SRCFILESSERVER))
TESTOBJFILES := $(patsubst $(TESTDIR)/%.c,$(TESTOBJDIR)/%.o,$(TESTFILES))
CINTAOBJFILES := $(patsubst $(CINTA)/%.c,$(CINTAOBJ)/%.o,$(CINTAFILES))

ALLFILES := $(SRCFILESCLIENT) $(SRCFILESSERVER) $(TESTFILES) $(shell find $(SRCDIR) $(TESTDIR) -type f -name "*.h")


# Create obj directory at the beginning
$(shell mkdir -p $(SRCOBJDIRCLIENT))
$(shell mkdir -p $(SRCOBJDIRSERVER))
$(shell mkdir -p $(TESTOBJDIR))
$(shell mkdir -p $(CINTAOBJ))


$(SRCOBJDIRCLIENT)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(SRCOBJDIRSERVER)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TESTOBJDIR)/%.o: $(TESTDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(CINTAOBJ)/%.o: $(CINTA)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all all_client all_server

all: $(EXEC_CLIENT) $(EXEC_SERVER)

all_client: $(EXEC_CLIENT)

all_server: $(EXEC_SERVER)

$(EXEC_CLIENT): $(OBJFILESCLIENT) 
	$(CC) -o $@ $^ $(CFLAGS)
	
$(EXEC_SERVER): $(OBJFILESSERVER) 
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: format fmt check-format

format fmt:
	clang-format -i $(ALLFILES)

check-format:
	clang-format --dry-run --Werror $(ALLFILES)


test: compile-tests
	./test 

.PHONY: test-valgrind

test-valgrind: compile-tests-valgrind 
	$(VALGRIND) $(VALGRIND_OPTS) ./test

.PHONY: compile-tests compile-tests-valgrind

compile-tests-valgrind: $(filter-out $(SRCOBJDIRCLIENT)/$(EXEC_CLIENT).o $(SRCOBJDIRSERVER), $(OBJFILESCLIENT)) $(TESTOBJFILES) $(CINTAOBJFILES)
	$(CC) -o $(TEST) $^ -g $(CFLAGS)

compile-tests: $(filter-out $(SRCOBJDIRCLIENT)/$(EXEC_CLIENT).o, $(OBJFILESCLIENT)) $(TESTOBJFILES) $(CINTAOBJFILES)
	$(CC) -o $(TEST) $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(EXEC_CLIENT) $(EXEC_SERVER) test



