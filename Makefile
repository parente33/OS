CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -pedantic -O2
DBGFLAG := -g                     # add "make DEBUG=1" to keep symbols

ifeq ($(DEBUG),1)
CFLAGS += $(DBGFLAG)
endif

ifdef CACHE_IMPL
CFLAGS += -DCACHE_IMPL=$(CACHE_IMPL)
endif

GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0 2>/dev/null)
GLIB_LIBS   := $(shell pkg-config --libs   glib-2.0 2>/dev/null)

CFLAGS += $(GLIB_CFLAGS)

INCLUDES := -Iinclude -Iinclude/common -Iinclude/server -Iinclude/server/doc -Iinclude/server/cache

# --------------------------------------------------------------------
#  Directory layout (fixed)
# --------------------------------------------------------------------
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
TMP_DIR := tmp

# --------------------------------------------------------------------
#  Do not compile cache implementation files separately – they are
#  already #included from src/server/cache/cache.c. Compiling them
#  again would duplicate the symbols (cache_init, cache_get, …) and
#  break the link step.
# --------------------------------------------------------------------
CACHE_IMPL_SRC := $(SRC_DIR)/server/cache/cache_lru.c \
                  $(SRC_DIR)/server/cache/cache_none.c

ALL_SRC      := $(filter-out $(CACHE_IMPL_SRC),$(shell find $(SRC_DIR) -name '*.c'))
CLIENT_SRC   := $(SRC_DIR)/dclient.c
SERVER_SRC   := $(filter-out $(CLIENT_SRC),$(ALL_SRC))
COMMON_SRC   := $(shell find $(SRC_DIR)/common -name '*.c')

# --------------------------------------------------------------------
#  Object lists (mirror src/ - obj/)
# --------------------------------------------------------------------
ALL_OBJ      := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(ALL_SRC))
CLIENT_OBJ   := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SRC) $(CLIENT_SRC))
SERVER_OBJ   := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SRC) $(SERVER_SRC))

# --------------------------------------------------------------------
#  Final binaries
# --------------------------------------------------------------------
CLIENT_BIN := $(BIN_DIR)/dclient
SERVER_BIN := $(BIN_DIR)/dserver

# --------------------------------------------------------------------
#  Phony targets
# --------------------------------------------------------------------
.PHONY: all clean dirs

all: dirs $(CLIENT_BIN) $(SERVER_BIN)

# Ensure output directories exist
# (dirs is implicit prerequisite for compilation & link rules)
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR) $(TMP_DIR)

# --------------------------------------------------------------------
#  Linking
# --------------------------------------------------------------------
$(CLIENT_BIN): $(CLIENT_OBJ) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(GLIB_LIBS) -o $@

$(SERVER_BIN): $(SERVER_OBJ) | dirs
	$(CC) $(CFLAGS) $(INCLUDES) $^ $(GLIB_LIBS) -o $@

# --------------------------------------------------------------------
#  Compilation (automatic mkdir for nested obj/ sub-dirs)
# --------------------------------------------------------------------
# Pattern rule – any obj/%.o depends on src/%.c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# --------------------------------------------------------------------
#  Auto-dependency inclusion
# --------------------------------------------------------------------
-include $(ALL_OBJ:.o=.d)

# --------------------------------------------------------------------
#  House-keeping
# --------------------------------------------------------------------
clean:
	@echo "Cleaning…"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)
