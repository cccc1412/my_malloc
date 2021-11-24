include project

CC = gcc

ifeq "$(VERSION)" "debug"
CFLAGS = -Wall -g3
else
CFLAGS = -O2
endif

FREESTANDING_FLAGS = -ffreestanding -nostartfiles -nostdlib

#
# Prepare for directories.
#
# INCLUDE_FLAG = -Iinclude1 -Iinclude2
INCLUDE_FLAG = $(patsubst %,-I%,$(INCLUDE_PATH))

#
# SRC_PATH_FILES = src1/a2.c src1/a1.c src2/b2.c src2/b1.c
SRC_PATH_FILES = $(foreach var,$(SRC_PATH),$(call ONE_DIR_FILES,$(var)))
ONE_DIR_FILES = $(wildcard $1/*.c)

#
# OBJ_DIR = obj/debug
OBJ_DIR = $(join $(addsuffix /,$(OBJ_PATH)),$(VERSION))

#
# OBJ_PATH_FILES = obj/debug/a2.o obj/debug/a1.o obj/debug/b2.o obj/debug/b1.o
OBJ_PATH_FILES = $(foreach var,$(SRC_PATH_FILES),$(call ONE_OBJ_FILE,$(var)))
ONE_OBJ_FILE = $(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(notdir $(basename $1))))

#
# BIN_DIR = bin/debug
BIN_DIR = $(join $(addsuffix /,$(BIN_PATH)),$(VERSION))

#
# OUTPUT_PATH_FILE = bin/debug/test
OUTPUT_PATH_FILE = $(join $(addsuffix /,$(BIN_DIR)),$(OUTPUT_FILE))

#
# Build directories.
#
ISOBJEXIST = $(if $(wildcard $(OBJ_PATH)),,$(shell mkdir $(OBJ_PATH)))
ISOBJDIREXIST = $(if $(wildcard $(OBJ_DIR)),,$(shell mkdir $(OBJ_DIR)))

#
ISBINEXIST = $(if $(wildcard $(BIN_PATH)),,$(shell mkdir $(BIN_PATH)))
ISBINDIREXIST = $(if $(wildcard $(BIN_DIR)),,$(shell mkdir $(BIN_DIR)))

#
# build
#
all : build_obj_bin_dir $(OUTPUT_PATH_FILE)

#
#
build_obj_bin_dir :
	$(ISOBJEXIST)
	$(ISOBJDIREXIST)
	$(ISBINEXIST)
	$(ISBINDIREXIST)

#
#
$(OUTPUT_PATH_FILE) : $(OBJ_PATH_FILES)
	$(CC) $(FREESTANDING_FLAGS) -e _start -o $@ $^

#
#
define OBJ_PRODUCE
SRC_NAME = $$(filter %$$(addprefix /,$$(notdir $$(basename $1))).c,$$(SRC_PATH_FILES))

# .c's dependencies head files
SRC_NAME_HEAD_FILES = $$(filter %.h,$$(shell $$(CC) $$(FREESTANDING_FLAGS) $$(INCLUDE_FLAG) -M $$(SRC_NAME)))

$1:$$(SRC_NAME) $$(SRC_NAME_HEAD_FILES)
	$$(CC) $$(FREESTANDING_FLAGS) -c -o $$@ $$(INCLUDE_FLAG) $$< $$(CFLAGS)
endef

$(foreach var,$(OBJ_PATH_FILES),$(eval $(call OBJ_PRODUCE,$(var))))

#
#
clean:
	rm $(OBJ_PATH_FILES)

#
#
.PHONY : clean build_obj_bin_dir debug

#
#
debug:
	@echo $(SRC_FILES)
	@echo $(INCLUDE_FLAG)
	@echo $(OBJ_FILES)
