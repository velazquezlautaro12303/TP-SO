CC = gcc
CFLAGS = -g -Wall# -Werror
LIBS = pthread readline commons
BIN_DIR = bin

# Nombre del archivo ejecutable
TARGETS = console cpu filesystem kernel memory

# Directorio de archivos comunes
COMMON_DIR = include
COMMON_FILES = $(wildcard $(COMMON_DIR)/*.c)
COMMON_OBJS = $(COMMON_FILES:.c=.o)

# Directorio de archivos fuente
SRC_DIR = src
SRC_FILES = $(foreach dir, $(SRC_DIR)/$(TARGETS), $(wildcard $(dir)/*.c))
OBJS = $(SRC_FILES:.c=.o)

# Variable que almacena la lista
LIST = $(COMMON_OBJS)

all: preparation $(foreach target, $(TARGETS), $(addprefix $(BIN_DIR)/, $(target)/$(target)))

preparation:
	$(shell mkdir -p $(addprefix $(BIN_DIR)/, $(TARGETS)))
	$(shell cd src && ./compiler.sh)

define build_target
LIST += $(BIN_DIR)/$(1)/main.o $(BIN_DIR)/$(1)/$(1)
$(BIN_DIR)/$(1)/main.o: $(SRC_DIR)/$(1)/main.c
	$(CC) $(CFLAGS) -I $(COMMON_DIR) -c $(SRC_DIR)/$(1)/main.c -o $$@
$(BIN_DIR)/$(1)/$(1): $(BIN_DIR)/$(1)/main.o $(COMMON_OBJS)
	$(CC) $$^ $(CFLAGS) -I $(COMMON_DIR) $(addprefix -l , $(LIBS)) -o $$@
endef

$(foreach target, $(TARGETS), $(eval $(call build_target,$(target))))

$(COMMON_OBJS): $(COMMON_DIR)/%.o : $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) $(addprefix -l , $(LIBS)) -I $(COMMON_DIR) -c $< -o $@

clean:
	rm -f $(LIST)

.PHONY: all clean preparation
