#include <iostream>
#include "include/lua.hpp"

#pragma comment (lib, "lua54.lib")
using namespace std;

int add_from_lua(lua_State* L)
{
	int a = lua_tonumber(L, -2);
	int b = lua_tonumber(L, -1);
	lua_pop(L, 3);
	int result = a + b;
	lua_pushnumber(L, result);
	return 1;
}

int main()
{
	const char* buff = "printt \"Hello from Lua.\"\n";

	lua_State* L = luaL_newstate();	// Lua ���� �ӽ��� �����.
	luaL_openlibs(L);	// �⺻ �Լ��� ����ϱ� ���� Open Library �ε�.

	//luaL_loadbuffer(L, buff, strlen(buff), "line");	// ���� �ӽſ� buff�� �ִ� ���ڿ� �ε�
	luaL_loadfile(L, "dragon_ai.lua");

	int error = lua_pcall(L, 0, 0, 0);	// lua_pcall�� ���� �����Ѵ�.
	if (error) {
		cout << "Error:" << lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	lua_getglobal(L, "pos_x");
	lua_getglobal(L, "pos_y");
	int pos_x = lua_tonumber(L, -2);	// stack
	int pos_y = lua_tonumber(L, -1);
	lua_pop(L, 2);

	cout << "pos = (" << pos_x << ", " << pos_y << ")\n";

	lua_getglobal(L, "plustwo");
	lua_pushnumber(L, 100);
	lua_pcall(L, 1, 1, 0);
	int result = lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "result : " << result << endl;

	lua_register(L, "c_add", add_from_lua);

	lua_getglobal(L, "lua_add");
	lua_pushnumber(L, 100);
	lua_pushnumber(L, 200);
	lua_pcall(L, 2, 1, 0);

	int sum = lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "sum : " << sum << endl;

	lua_close(L);	// newstate�� ���� �޸𸮸� �Ҵ�޾����Ƿ� �����Ѵ�.
}