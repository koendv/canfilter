# Makefile for CAN filter program + man page + cross compilation

# Program
CC_LINUX = gcc
CC_WIN   = x86_64-w64-mingw32-gcc
CFLAGS   = -O2 -Wall -Wextra
TARGET   = canfilter
LICENSE  = LICENSE.md

# Man page
DOC_SRC = canfilter.md
DOC_MAN = canfilter.1
DOC_PDF = canfilter.pdf

# Sources
SRC      = canfilter.c

.PHONY: all clean pdf linux win actions

all: linux win pdf man

# Compile for Linux
linux: $(TARGET)

$(TARGET): $(SRC)
	$(CC_LINUX) $(CFLAGS) -o $@ $^

# Compile for Windows
win: $(SRC)
	$(CC_WIN) $(CFLAGS) -o $(TARGET).exe $^

# Generate man page from markdown
man: $(DOC_SRC)
	pandoc $(DOC_SRC) -s -t man -o $(DOC_MAN)

# Generate PDF from markdown
pdf: $(DOC_SRC)
	 pandoc $(DOC_SRC) -o $(DOC_PDF) -V geometry:a4paper -V geometry:margin=2cm -V fontsize=11pt

# Clean all generated files
clean:
	rm -f $(TARGET) $(TARGET).exe $(DOC_PDF) $(DOC_MAN)

# GitHub Actions style
actions: clean all 

# Run self-tests
test: linux
	./$(TARGET) --selftest
	./$(TARGET) --std --output stm 0x100 0x200-0x20F
	./$(TARGET) --ext --output slcan 0x1000000

# Update package creation in build.yml
package: all
	zip -r9 canfilter.zip $(TARGET) $(TARGET).exe $(DOC_MAN) $(DOC_PDF) README.md $(LICENSE)
