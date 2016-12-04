//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

#include "definitions.h"
#include "configmanager.h"

ConfigManager::ConfigManager()
{
	m_loaded = false;
}

ConfigManager::~ConfigManager()
{
	//
}

bool ConfigManager::load()
{
	lua_State* L = luaL_newstate();
	if(!L)
		throw std::runtime_error("Failed to allocate memory");

	luaL_openlibs(L);

	if(luaL_dofile(L, "config.lua"))
	{
		std::cout << "[ERROR] " << lua_tostring(L, -1) << std::endl;
		lua_close(L);
		return false;
	}

	//parse config
	if(!m_loaded) //info that must be loaded one time (unless we reset the modules involved)
	{
		string[IP] = getGlobalString(L, "ip", "127.0.0.1");
		string[GAMEIP] = getGlobalString(L, "gameIp", "127.0.0.1");
		string[MYSQL_HOST] = getGlobalString(L, "mysqlHost", "localhost");
		string[MYSQL_USER] = getGlobalString(L, "mysqlUser", "root");
		string[MYSQL_PASS] = getGlobalString(L, "mysqlPass", "");
		string[MYSQL_DB] = getGlobalString(L, "mysqlDatabase", "realots");

		integer[PORT] = getGlobalNumber(L, "port", 17778);
		integer[GAMEPORT] = getGlobalNumber(L, "gamePort", 7172);
		integer[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers", 1500);
		integer[MAX_PLAYERS_NEWBIES] = getGlobalNumber(L, "maxPlayersNewbies", 500);
		integer[PREM_BUFFER] = getGlobalNumber(L, "premBuffer", 900);
		integer[RESERVED_PREM_NEWBIES] = getGlobalNumber(L, "reservedPremiumNewbies", 250);
		integer[SERVERSAVE_H] = getGlobalNumber(L, "serverSaveHour", 5);
		integer[WORLD_TYPE] = getGlobalNumber(L, "worldType", 0);

		///boolean[FIRST_BOOLEAN_CONFIG] = getGlobalBoolean(L, "firstBooleanConfig", true);
	}
	
	m_loaded = true;
	lua_close(L);
	return true;
}

bool ConfigManager::reload()
{
	if(!m_loaded)
		return false;

	return load();
}

const std::string& ConfigManager::getString(uint32_t _what) const
{ 
	if(m_loaded && _what < LAST_STRING_CONFIG)
		return string[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getString] " << _what << std::endl;
		return string[DUMMY_STR];
	}
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_loaded && _what < LAST_INTEGER_CONFIG)
		return integer[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}
bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(m_loaded && _what < LAST_INTEGER_CONFIG)
	{
		integer[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1))
		return _default;

	int32_t len = (int32_t)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L,1);

	return ret;
}

int32_t ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	int32_t val = (int32_t)lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

std::string ConfigManager::getGlobalStringField (lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default) {
	lua_getglobal(_L, _identifier.c_str());

	lua_pushnumber(_L, _key);
	lua_gettable(_L, -2);  /* get table[key] */
	if(!lua_isstring(_L, -1))
		return _default;
	std::string result = lua_tostring(_L, -1);
	lua_pop(_L, 2);  /* remove number and key*/
	return result;
}

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_loaded && _what < LAST_BOOLEAN_CONFIG)
		return boolean[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getBool] " << _what << std::endl;
		return false;
	}
}

bool ConfigManager::getGlobalBoolean(lua_State* L, const char* identifier, const bool defaultValue)
{
	lua_getglobal(L, identifier);
	if (!lua_isboolean(L, -1)) {
		if (!lua_isstring(L, -1)) {
			return defaultValue;
		}

		size_t len = lua_strlen(L, -1);
		std::string str(lua_tostring(L, -1), len);
		lua_pop(L, 1);
		return (str == "yes" || str == "true" || str == "y" || atoi(str.c_str()) > 0);
	}

	int val = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return val != 0;
}
