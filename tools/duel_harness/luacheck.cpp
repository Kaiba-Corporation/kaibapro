#include <lua.h>
#include <lauxlib.h>
#include <cstdio>
int main(int argc, char** argv) {
	int bad = 0;
	for (int i = 1; i < argc; ++i) {
		lua_State* L = luaL_newstate();
		if (luaL_loadfile(L, argv[i]) != LUA_OK) { std::printf("BAD %s\n", lua_tostring(L, -1)); ++bad; }
		lua_close(L);
	}
	std::printf("%d files, %d bad\n", argc - 1, bad);
	return bad;
}
