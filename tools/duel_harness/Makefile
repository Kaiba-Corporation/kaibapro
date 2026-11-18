# Builds the duel harness against an engine folder (Linux).
#   make CORE=path/to/engine LUA=path/to/lua-5.4/src OUT=build
# CORE is a complete kaibapro-core source folder: the pinned commit with bot_theater_core
# copied over it (on the four-player branch, bot_theater_core alone is complete).
# LUA is the Lua 5.4 source folder; it is compiled as C++, like the game does.

CORE ?= ../../bot_theater_core
LUA ?= lua
OUT ?= build
HERE := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CORE_SRC := $(wildcard $(CORE)/*.cpp)
LUA_SRC := $(filter-out $(LUA)/lua.c $(LUA)/luac.c $(LUA)/onelua.c $(LUA)/linit.c,$(wildcard $(LUA)/*.c))
CORE_OBJ := $(patsubst $(CORE)/%.cpp,$(OUT)/core/%.o,$(CORE_SRC))
LUA_OBJ := $(patsubst $(LUA)/%.c,$(OUT)/lua/%.o,$(LUA_SRC))
OPT ?= -O2 -g

all: $(OUT)/harness $(OUT)/luacheck

$(OUT)/lua/%.o: $(LUA)/%.c
	@mkdir -p $(dir $@)
	g++ -x c++ $(OPT) -DLUA_USE_LINUX -w -c $< -o $@

$(OUT)/core/%.o: $(CORE)/%.cpp $(wildcard $(CORE)/*.h)
	@mkdir -p $(dir $@)
	g++ -std=c++14 $(OPT) -DLUA_USE_LINUX -w -I$(LUA) -c $< -o $@

$(OUT)/libcore.a: $(CORE_OBJ) $(LUA_OBJ)
	rm -f $@
	ar rcs $@ $^

$(OUT)/harness: $(HERE)harness.cpp $(OUT)/libcore.a
	g++ -std=c++17 $(OPT) -w -I$(CORE) -I$(LUA) $< -o $@ $(OUT)/libcore.a -lsqlite3

$(OUT)/luacheck: $(HERE)luacheck.cpp $(OUT)/libcore.a
	g++ -std=c++17 $(OPT) -w -I$(LUA) $< -o $@ $(OUT)/libcore.a

clean:
	rm -rf $(OUT)

.PHONY: all clean
