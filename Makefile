CC=gcc
CFLAGS=-Wall -Wextra
EXEC=main

SRCDIR=src
OBJDIR=obj

SRCFILES := $(shell find $(SRCDIR) -name "*.c")
ALLFILES := $(SRCFILES) $(shell find $(SRCDIR) -name "*.h")
OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCFILES))


# Create obj directory at the beginning
$(shell mkdir -p $(OBJDIR))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)


.PHONY: all 

all: $(EXEC)

main: $(OBJFILES) 
	$(CC) -o $@ $^ $(CFLAGS)
	
.PHONY: format fmt

format fmt:
	clang-format -i $(ALLFILES)

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(EXEC) test



