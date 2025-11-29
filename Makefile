# ============================================
#   PROJECT SETTINGS
# ============================================
PROJECT := canfilter
BIN_LINUX := $(PROJECT)
BIN_WIN := $(PROJECT).exe

SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
WIN_OBJ_DIR := obj_win

WIN_INC_DIR := windows/include
WIN_LIB_DIR := windows/lib

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
HDRS := $(wildcard $(INC_DIR)/*.hpp)

OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
WIN_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(WIN_OBJ_DIR)/%.o,$(SRCS))

MANPAGE := canfilter.1
DOC_MD := docs/canfilter.md


# ============================================
#   COMPILERS
# ============================================
CXX := g++
CXXFLAGS := -g -std=c++20 -Wall -Wextra -I$(INC_DIR)

# Linux: dynamic link
LDLIBS_LINUX := -lusb-1.0

# Windows cross compiler
CXX_WIN := x86_64-w64-mingw32-g++
CXXFLAGS_WIN := -g -std=c++20 -Wall -Wextra -I$(INC_DIR) -I$(WIN_INC_DIR)

# Windows: static link
LDLIBS_WIN := $(WIN_LIB_DIR)/libusb-1.0.a -static


# ============================================
#   DEFAULT TARGET
# ============================================
all: linux windows man


# ============================================
#   LINUX BUILD
# ============================================
linux: $(BIN_LINUX)

$(BIN_LINUX): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS_LINUX)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


# ============================================
#   WINDOWS CROSS-COMPILATION
# ============================================
windows: $(BIN_WIN)

$(BIN_WIN): $(WIN_OBJS)
	$(CXX_WIN) $(CXXFLAGS_WIN) -o $@ $^ $(LDLIBS_WIN)

$(WIN_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(WIN_OBJ_DIR)
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

$(WIN_OBJ_DIR):
	mkdir -p $(WIN_OBJ_DIR)


# ============================================
#   FORMAT SOURCE CODE
# ============================================
format:
	clang-format -i $(SRCS) $(HDRS)


# ============================================
#   MAN PAGE GENERATION USING PANDOC
# ============================================
man: $(MANPAGE)

$(MANPAGE): $(DOC_MD)
	pandoc -s $(DOC_MD) -t man -o $(MANPAGE)


# ============================================
#   PACKAGE CREATION (ZIP)
# ============================================
package: all
	rm -f $(PROJECT).zip
	mkdir -p $(PROJECT)-package
	cp	$(BIN_LINUX) \
		$(BIN_WIN) \
		README.md \
		LICENSE.md \
		$(MANPAGE) \
		$(DOC_MD) \
                $(PROJECT)-package
	zip -r9 $(PROJECT).zip $(PROJECT)-package

# ============================================
#   CLEAN
# ============================================
clean:
	rm -rf $(OBJ_DIR) $(WIN_OBJ_DIR) $(BIN_LINUX) $(BIN_WIN) $(MANPAGE) $(PROJECT).zip $(PROJECT)-package

.PHONY: all linux windows format man clean package

