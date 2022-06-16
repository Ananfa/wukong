package.cpath = 'hotfix/?.so;'

local fixecho = require "libfixecho"

function fix_1000(a,b,c)
	return fixecho.lua_echo(a, b, c)
end