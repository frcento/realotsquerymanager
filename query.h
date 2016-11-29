#ifdef _WIN32
#pragma warning(disable:4786)
#endif
/*
 **     Query.h
 **
 **     Published / author: 2001-02-15 / grymse@alhem.net
 **/

/*
Copyright (C) 2001  Anders Hedstrom

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef _QUERY_H
#define _QUERY_H

#include <string>
#include <map>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


class Query 
{
public:
        Query(Database& dbin);
        Query(Database& dbin,const std::string& sql);
        ~Query();

        bool Connected();
        Database& GetDatabase() const;
        const std::string& GetLastQuery();

        bool execute(const std::string& sql);
        MYSQL_RES *get_result(const std::string& sql);

        void free_result();
        MYSQL_ROW fetch_row();
        my_ulonglong insert_id();
        long num_rows();
        int num_cols();
        std::string GetError();
        int GetErrno();

        bool is_null(const std::string& x);
        bool is_null(int x);
        bool is_null();

        const char *get_string(const std::string& sql);
        long get_count(const std::string& sql);
        double get_num(const std::string& sql);

        const char *getstr(const std::string& x);
        const char *getstr(int x);
        const char *getstr();
        long getval(const std::string& x);
        long getval(int x);
        long getval();
        unsigned long getuval(const std::string& x);
        unsigned long getuval(int x);
        unsigned long getuval();
        int64_t getbigint(const std::string& x);
        int64_t getbigint(int x);
        int64_t getbigint();
        uint64_t getubigint(const std::string& x);
        uint64_t getubigint(int x);
        uint64_t getubigint();
        double getnum(const std::string& x);
        double getnum(int x);
        double getnum();

protected:
        Query(Database *dbin);
        Query(Database *dbin,const std::string& sql);
private:
        Query(const Query& q) : m_db(q.GetDatabase()) {}
        Query& operator=(const Query& ) { return *this; }
        void error(const std::string& );
        Database& m_db;
        Database::OPENDB *odb;
        MYSQL_RES *res;
        MYSQL_ROW row;
        short rowcount;
        std::string m_tmpstr;
        std::string m_last_query;
        std::map<std::string,int> m_nmap;
        int m_num_cols;
};


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

#endif // _QUERY_H
