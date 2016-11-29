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

#ifndef _DATABASE_H
#define _DATABASE_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <string>
#include <list>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

class IError;
class Query;
class Mutex;


class Database 
{
public:
        class Mutex {
        public:
                Mutex();
                ~Mutex();
                void Lock();
                void Unlock();
        private:
#ifdef _WIN32
                HANDLE m_mutex;
#else
                pthread_mutex_t m_mutex;
#endif
        };
private:
        class Lock {
        public:
                Lock(Mutex& mutex,bool use);
                ~Lock();
        private:
                Mutex& m_mutex;
                bool m_b_use;
        };
public:
        struct OPENDB {
                OPENDB() : busy(false) {}
                MYSQL mysql;
                bool busy;
        };
        typedef std::list<OPENDB *> opendb_v;

public:
        Database(const std::string& database,
                IError * = NULL);

        Database(Mutex& ,const std::string& database,
                IError * = NULL);

        Database(const std::string& host,
                const std::string& user,
                const std::string& password = "",
                const std::string& database = "",
                IError * = NULL);

        Database(Mutex& ,const std::string& host,
                const std::string& user,
                const std::string& password = "",
                const std::string& database = "",
                IError * = NULL);

        virtual ~Database();

        virtual void OnMyInit(OPENDB *);

        bool Connected();

        void RegErrHandler(IError *);
        void error(Query&,const char *format, ...);

        OPENDB *grabdb();
        void freedb(OPENDB *odb);

        // utility
        std::string safestr(const std::string& );
        std::string unsafestr(const std::string& );
        std::string xmlsafestr(const std::string& );

        int64_t a2bigint(const std::string& );
        uint64_t a2ubigint(const std::string& );
        static std::string escapeString(const std::string &s);
        static std::string escapeString(const char* s, unsigned long size);

private:
        Database(const Database& ) : m_mutex(m_mutex) {}
        Database& operator=(const Database& ) { return *this; }
        void error(const char *format, ...);
        //
        std::string host;
        std::string user;
        std::string password;
        std::string database;
        opendb_v m_opendbs;
        IError *m_errhandler;
        bool m_embedded;
        Mutex& m_mutex;
        bool m_b_use_mutex;
};

#ifdef MYSQLW_NAMESPACE
} //namespace MYSQLW_NAMESPACE {
#endif


#endif // _DATABASE_H
