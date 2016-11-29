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
#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include <string>
#include <map>
#ifdef WIN32
#include <config-win.h>
#include <mysql.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#endif

#include "database.h"
#include "query.h"


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


Query::Query(Database *dbin)
:m_db(*dbin)
,odb(dbin ? dbin -> grabdb() : NULL)
,res(NULL)
,row(NULL)
,m_num_cols(0)
{
}


Query::Query(Database& dbin) : m_db(dbin),odb(dbin.grabdb()),res(NULL),row(NULL)
,m_num_cols(0)
{
}


Query::Query(Database *dbin,const std::string& sql) : m_db(*dbin)
,odb(dbin ? dbin -> grabdb() : NULL),res(NULL),row(NULL)
,m_num_cols(0)
{
        execute(sql);
}


Query::Query(Database& dbin,const std::string& sql) : m_db(dbin),odb(dbin.grabdb()),res(NULL),row(NULL)
,m_num_cols(0)
{
        execute(sql); // returns 0 if fail
}


Query::~Query()
{
        if (res)
        {
                GetDatabase().error(*this, "mysql_free_result in destructor");
                mysql_free_result(res);
        }
        if (odb)
        {
                m_db.freedb(odb);
        }
}


Database& Query::GetDatabase() const
{
        return m_db;
}


bool Query::execute(const std::string& sql)
{               // query, no result
        m_last_query = sql;
        if (odb && res)
        {
                GetDatabase().error(*this, "execute: query busy");
        }
        if (odb && !res)
        {
                if (mysql_query(&odb -> mysql,sql.c_str()))
                {
                        GetDatabase().error(*this,"query failed");
                }
                else
                {
                        return true;
                }
        }
        return false;
}



// methods using db specific api calls

MYSQL_RES *Query::get_result(const std::string& sql)
{       // query, result
	//std::cout << "Query: " << sql << std::endl;

        if (odb && res)
        {
                GetDatabase().error(*this, "get_result: query busy");
        }
        if (odb && !res)
        {
                if (execute(sql))
                {
                        res = mysql_store_result(&odb -> mysql);
                        if (res)
                        {
                                MYSQL_FIELD *f = mysql_fetch_field(res);
                                int i = 1;
                                while (f)
                                {
                                        if (f -> name)
                                                m_nmap[f -> name] = i;
                                        f = mysql_fetch_field(res);
                                        i++;
                                }
                                m_num_cols = i - 1;
                        }
                }
        }
        return res;
}


void Query::free_result()
{
        if (odb && res)
        {
                mysql_free_result(res);
                res = NULL;
                row = NULL;
        }
        while (m_nmap.size())
        {
                std::map<std::string,int>::iterator it = m_nmap.begin();
                m_nmap.erase(it);
        }
        m_num_cols = 0;
}


MYSQL_ROW Query::fetch_row()
{
        rowcount = 0;
        return odb && res ? row = mysql_fetch_row(res) : NULL;
}


my_ulonglong Query::insert_id()
{
        if (odb)
        {
                return mysql_insert_id(&odb -> mysql);
        }
        else
        {
                return 0;
        }
}


long Query::num_rows()
{
        return odb && res ? mysql_num_rows(res) : 0;
}


int Query::num_cols()
{
        return m_num_cols;
}


bool Query::is_null(int x)
{
        if (odb && res && row)
        {
                return row[x] ? false : true;
        }
        return false; // ...
}


bool Query::is_null(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return is_null(index);
        error("Column name lookup failure: " + x);
        return false;
}


bool Query::is_null()
{
        return is_null(rowcount++);
}


const char *Query::getstr(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getstr(index);
        error("Column name lookup failure: " + x);
        return NULL;
}


const char *Query::getstr(int x)
{
        if (odb && res && row)
        {
                return row[x] ? row[x] : "";
        }
        return NULL;
}


const char *Query::getstr()
{
        return getstr(rowcount++);
}


double Query::getnum(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getnum(index);
        error("Column name lookup failure: " + x);
        return 0;
}


double Query::getnum(int x)
{
        return odb && res && row && row[x] ? atof(row[x]) : 0;
}


long Query::getval(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getval(index);
        error("Column name lookup failure: " + x);
        return 0;
}


long Query::getval(int x)
{
        return odb && res && row && row[x] ? atol(row[x]) : 0;
}


double Query::getnum()
{
        return getnum(rowcount++);
}


long Query::getval()
{
        return getval(rowcount++);
}


unsigned long Query::getuval(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getuval(index);
        error("Column name lookup failure: " + x);
        return 0;
}


unsigned long Query::getuval(int x)
{
        unsigned long l = 0;
        if (odb && res && row && row[x])
        {
                l = m_db.a2ubigint(row[x]);
        }
        return l;
}


unsigned long Query::getuval()
{
        return getuval(rowcount++);
}


int64_t Query::getbigint(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getbigint(index);
        error("Column name lookup failure: " + x);
        return 0;
}


int64_t Query::getbigint(int x)
{
        return odb && res && row && row[x] ? m_db.a2bigint(row[x]) : 0;
}


int64_t Query::getbigint()
{
        return getbigint(rowcount++);
}


uint64_t Query::getubigint(const std::string& x)
{
        int index = m_nmap[x] - 1;
        if (index >= 0)
                return getubigint(index);
        error("Column name lookup failure: " + x);
        return 0;
}


uint64_t Query::getubigint(int x)
{
        return odb && res && row && row[x] ? m_db.a2ubigint(row[x]) : 0;
}


uint64_t Query::getubigint()
{
        return getubigint(rowcount++);
}


double Query::get_num(const std::string& sql)
{
        double l = 0;
        if (get_result(sql))
        {
                if (fetch_row())
                {
                        l = getnum();
                }
                free_result();
        }
        return l;
}


long Query::get_count(const std::string& sql)
{
        long l = 0;
        if (get_result(sql))
        {
                if (fetch_row())
                        l = getval();
                free_result();
        }
        return l;
}


const char *Query::get_string(const std::string& sql)
{
        bool found = false;
        m_tmpstr = "";
        if (get_result(sql))
        {
                if (fetch_row())
                {
                        m_tmpstr = getstr();
                        found = true;
                }
                free_result();
        }
        return m_tmpstr.c_str(); // %! changed from 1.0 which didn't return NULL on failed query
}


const std::string& Query::GetLastQuery()
{
        return m_last_query;
}


std::string Query::GetError()
{
        return odb ? mysql_error(&odb -> mysql) : "";
}


int Query::GetErrno()
{
        return odb ? mysql_errno(&odb -> mysql) : 0;
}


bool Query::Connected()
{
        if (odb)
        {
                if (mysql_ping(&odb -> mysql))
                {
                        GetDatabase().error(*this, "mysql_ping() failed");
                        return false;
                }
        }
        return odb ? true : false;
}


void Query::error(const std::string& x)
{
        m_db.error(*this, x.c_str());
}


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

