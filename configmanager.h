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

#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <string>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class ConfigManager
{
	public:
		ConfigManager();
		~ConfigManager();

		enum boolean_config_t
		{
			FIRST_BOOLEAN_CONFIG = 0,
			LAST_BOOLEAN_CONFIG /* this must be the last one */
		};

		enum string_config_t 
		{
			DUMMY_STR = 0,
			IP,
			GAMEIP,
			MYSQL_HOST,
			MYSQL_USER,
			MYSQL_PASS,
			MYSQL_DB,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum integer_config_t
		{
			PORT = 0,
			GAMEPORT,
			SERVERSAVE_H,
			LAST_INTEGER_CONFIG /* this must be the last one */
		};

		bool load();
		bool reload();
		bool isLoaded() const {return m_loaded;}

		const std::string& getString(uint32_t _what) const;
		int32_t getNumber(uint32_t _what) const;
		bool setNumber(uint32_t _what, int32_t _value);
		bool getBool(uint32_t _what) const;

	private:
		std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default="");
		int32_t getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default=0);
		std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default="");
		static bool getGlobalBoolean(lua_State* L, const char* identifier, const bool defaultValue);

		bool m_loaded;
		std::string string[LAST_STRING_CONFIG];
		int32_t integer[LAST_INTEGER_CONFIG];
		bool boolean[LAST_BOOLEAN_CONFIG];
};

#endif /* _CONFIG_MANAGER_H */
