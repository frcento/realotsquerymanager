/*
*   OTServ QueryManager, (c) 2011 Satan Claus Limited
*/

#include "configmanager.h"
#include "definitions.h"
#include "networkmessage.h"

#include <cstring>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <mysql/mysql.h>
#include "database.h"
#include "query.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <ctime>    // For time()
#include <cstdlib>  // For srand() and rand()

#ifndef WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif

using namespace std;
ConfigManager g_config;

void log (std::string foo)
{
	ofstream myfile;
	myfile.open ("debug.txt");
	myfile << foo << std::endl;
	myfile.close();
}

std::string int2str(int i)
{
	std::stringstream sstream;
	sstream << i;
	std::string str;
	sstream >> str;
	return(str);
}

std::string str2str(std::string i)
{
	std::stringstream sstream;
	sstream << i;
	std::string str;
	sstream >> str;
	return(str);
}

THREAD_RETURN ConnectionHandler(void* dat);

/* Database Connection */

std::string host = "localhost";
std::string username = "root";
std::string password = "";
std::string database = "realots";

int main(int argc, char *argv[])
{
	std::cout << "--------------------" << std::endl;
	std::cout << "OTServ QueryManager" << std::endl;
	std::cout << "--------------------" << std::endl;

	// read global config
	std::cout << ":: Loading config... ";
	if(!g_config.load())
	{
		std::cout << "[ERROR] Unable to load config.lua!" << std::endl;
		return 0;
	}
	
	host = g_config.getString(ConfigManager::MYSQL_HOST);
	username = g_config.getString(ConfigManager::MYSQL_USER);
	password = g_config.getString(ConfigManager::MYSQL_PASS);
	database = g_config.getString(ConfigManager::MYSQL_DB);

	std::cout << "[done]" << std::endl;
	
	// connect to database
	std::cout << ":: Connecting to the MySQL database... ";
	Database db(host,username,password,database);
	if(!db.Connected())
	{
		std::cout << "[ERROR]" << std::endl;
		std::cout << "Failed to connect to database, read doc/MYSQL_HELP for information." << std::endl;
		return 0;
	}
	std::cout << "[done]" << std::endl;

	std::cout << ":: Starting OTServ QueryManager... ";

	// start the server listen...
	while(true)
	{
		sockaddr_in local_adress;
		memset(&local_adress, 0, sizeof(sockaddr_in)); // zero the struct

		local_adress.sin_family      = AF_INET;
		local_adress.sin_port        = htons(g_config.getNumber(ConfigManager::PORT));
		local_adress.sin_addr.s_addr = inet_addr((g_config.getString(ConfigManager::IP)).c_str());

		// first we create a new socket
		#ifdef WIN32
		SOCKET listen_socket;
		WSADATA wsa;

    	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    	{
        	printf("Failed. Error Code : %d",WSAGetLastError());
        	return 1;
    	}
		
		if(INVALID_SOCKET == (listen_socket = socket(AF_INET, SOCK_STREAM, 0))){
			std::cout << listen_socket << std::endl;
			return -1;
		}
		#else
		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);

		if(listen_socket <= 0){
			return -1;
		}
		#endif

		int yes = 1;
		// lose the pesky "Address already in use" error message
		#ifdef WIN32
		if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) == -1)
		#else
		if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)
		#endif
		{
			std::cout << "Unable to set socket options!" << std::endl;
			return -1;
		}
	
		//fcntl (listen_socket, F_SETFL, O_NONBLOCK);
		// bind socket on port
		if(bind(listen_socket, (struct sockaddr*)&local_adress, sizeof(struct sockaddr_in)) < 0){
			std::cout << "Unable to create server socket (2)!" << std::endl;
			return -1;
		}

		// now we start listen on the new socket
		if(listen(listen_socket, 10) == SOCKET_ERROR){
			std::cout << "Listen on server socket not possible!" << std::endl;
			return -1;
		}

		std::cout << "[done]" << std::endl;
		
		while(true)
		{
			fd_set listen_set;
			timeval tv;
			FD_ZERO(&listen_set);
			FD_SET(listen_socket, &listen_set);
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			int ret = select(listen_socket + 1, &listen_set, NULL, NULL, &tv);

			if(ret == SOCKET_ERROR){
				int errnum;
				errnum = errno;
				if(errnum != ERROR_EINTR){
					std::cout << "WARNING: select() function returned an error." << std::endl;
					break;
				}
			}
	
			else if(ret == 0)
				continue;

			SOCKET s = accept(listen_socket, NULL, NULL); // accept a new connection
			SLEEP(100);

			if(s > 0)
				CREATE_THREAD(ConnectionHandler, (void*)&s);
			else
				std::cout << "WARNING: Not a valid socket from accept() function." << std::endl;
		}

		closesocket(listen_socket);
	}
}

void parseDebug(NetworkMessage& msg)
{
	int dataLength = msg.getMessageLength() - 1;
	if(dataLength != 0)
	{
		std::cout << "data: ";
		int data = msg.InspectByte();
	
		for(int i = 1; i < dataLength; ++i){
			std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << (int)data << " ";
			data = msg.InspectByte(i);
		}
	
		std::cout << std::endl;
	}
}

THREAD_RETURN ConnectionHandler(void* dat)
{

/* Variables */

	int result;
	long accId;
	long randomAcc;
	long premDays;
	long trialDays;
	int userlevel;
	bool is_banished;
	int gender;
	int guild_id;
	int rank;
	int active;
	int buddys;
	long accountID;
	long accountNR;
	long progression;
	int initiatingId;
	int bandays;
	int loginError;
	int multiclient;
	std::string guild_name;
	std::string guild_title;
	std::string player_title;

	SOCKET socket = *(SOCKET*)dat;

	NetworkMessage msg;
	while(msg.ReadFromSocket(socket))
	{
		unsigned char packetId = msg.getByte();

		/* Authentication */
		std::cout << std::endl/* << "Packet Type: " << (int) packetId << std::endl*/;
		//parseDebug(msg);
		if(packetId == 0x00)
		{
			std::cout << "Authentication packet (0x00)" << std::endl;

			unsigned char unknown = msg.getByte(); //0x01
			std::string password = msg.getString(); //a6glaf0c
			std::string worldname = msg.getString();

			/*std::cout << "unknown: " << (int) unknown << std::endl;
			std::cout << "password: " << password << std::endl;
			std::cout << "worldname: " << worldname << std::endl;*/

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x14)
		{
			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Received playerLogin packet (0x14)" << std::endl;
			NetworkMessage writeMsg;
			unsigned int accountNumber = msg.getU32();
			std::string charName = msg.getString();
			std::string charPass = msg.getString();
			std::string charIP   = msg.getString();

			std::cout << "accountNumber: " << dec << accountNumber << std::endl;
			std::cout << "charName     : " << charName << std::endl;
			std::cout << "charPass     : " << charPass << std::endl;
			std::cout << "charIP       : " << charIP << std::endl;
			std::cout << ":: Authenticating against database" << std::endl;

			// Init some vars
			bool banned = false;
			unsigned int banTime = 0;
			bool banDelete = false;
			accId = 0;
			gender = 0;

			q.get_result("SELECT users.banished, users.banished_until, users.bandelete, players.account_id, players.gender FROM users,players WHERE users.login='" + int2str(accountNumber) + "' AND players.charname='" + Database::escapeString(charName.c_str()) + "' AND users.passwd = '" + Database::escapeString(str2str(charPass)) + "' AND users.login=players.account_nr");
			if (q.fetch_row())
			{
				banned = q.getval();
				banTime = q.getval();
				banDelete = q.getval();
				accId  = q.getval();
				gender = q.getval();
				q.free_result();
			}

			/*
			* Perform some basic checks
			*/

			loginError = 0;

			if(banned && banTime <= time(NULL))
			{
				q.execute("UPDATE users SET banished='0' WHERE login='" + int2str(accountNumber) + "'");
				q.free_result();
				std::cout << "Banishment of account " + int2str(accountNumber) + " has expired " << (time(NULL) - banTime) << " seconds ago, removing." << std::endl;
				banned = false;
			}
	
			if (!accId || accId == 0)
				loginError = 0x06;
			else if(banDelete || banned)
				loginError = 0x0A;
			else {
				q.get_result("SELECT 1 FROM players WHERE account_nr = '" + int2str(accountNumber) + "' AND online='1' AND account_id<>'"+int2str(accId)+" LIMIT 1'");
		
				if (q.fetch_row())
					loginError = 0x0D;

				q.free_result();
			}

		/*
			0x01 = doesnt exist
			0x02 = doesnt exist
			0x03 = doesnt live on this world
			0x04 = Private
			0x05 = ???
			0x06 = Accountnumber or password incorrect
			0x07 = Account disabled for 5 minutes
			0x08 = 0x06
			0x09 = IP Address blocked for 30 minutes
			0x0A = Banished
			0x0B = Banished for name
			0x0C = IP Banned
			0x0D = MCing
			0x0E = May only login with GM acc
			0x0F = Login failure due to corrupted data
		*/

			if (loginError > 0 && loginError != 13)
			{
				std::cout << ":: LOGIN FAILURE " << dec << loginError << " for " << dec << accId << std::endl;
				writeMsg.addByte(0x01);
				writeMsg.addByte(loginError); // Significant byte (0x02 is not exist, 0x03 not on this world)
				writeMsg.WriteToSocket(socket);
			}
			else if (accId)
			{
				// Begin of login block
				std::cout << ":: Login succes from accountId " << dec << accId << std::endl;
				writeMsg.addByte(0x00);
				writeMsg.addU32(accId);
				writeMsg.addString(charName.c_str()) ;
				writeMsg.addByte(gender == 1 ? 0x01 : 0x02);

				guild_id = 0;
				rank = 0;
				guild_name = "";
				guild_title = "0";
				player_title = "0";
				active = 0;

				q.execute("UPDATE players SET online='1', charIP='"+Database::escapeString(charIP)+"' WHERE account_id=" + int2str(accId));
				q.free_result();
				q.get_result("SELECT guild_id,rank,player_title FROM guild_members WHERE invitation='0' AND account_id = '" + int2str(accId) + "'" );
				
				if (q.fetch_row())
				{
					guild_id = q.getval();
					rank = q.getval();
					//guild_title = q.getstr();
					player_title = q.getstr();
				}
				
				q.free_result();
				if (rank > 0)
				{
					q.get_result("SELECT rank_" + int2str(rank) + " FROM guilds WHERE guild_id = '" + int2str(guild_id) + "'" );
					if (q.fetch_row())
					{
						guild_title = q.getstr();
					}
					q.free_result();
				}
				
				q.get_result("SELECT COUNT(*) FROM guild_members WHERE invitation='0' AND guild_id = '" + int2str(guild_id) + "'");
				int active = 0;
				
				if(q.fetch_row())
					active = q.getval();
		
				q.free_result();

				if(active)
					std::cout << "Active members in the guild: " << dec << active << std::endl;
		
				bool none = true;
				if (active > 2)
				{
					q.get_result("SELECT guild_name FROM guilds WHERE guild_id = " + int2str(guild_id));
					if(q.fetch_row())
					{
						guild_name = q.getstr();
						writeMsg.addString(guild_name.c_str()); // Guild
						writeMsg.addString(guild_title.c_str()); // Guild Title
						writeMsg.addString(player_title.c_str()); // Player Title
						none = false;
					}
					q.free_result();
				}

				if(none)
				{
					writeMsg.addString(""); // Guild
					writeMsg.addString("0"); // Guild Title
					writeMsg.addString("0"); // Player Title
				}

				/* Get buddies */
				q.get_result("SELECT buddy_id,buddy_string FROM buddy WHERE account_nr = " + int2str(accountNumber) + " LIMIT 100");
				buddys = q.num_rows();

				writeMsg.addByte(buddys);
				if(buddys)
				{
					std::cout << "About to load " << dec << buddys << " buddys." << std::endl;
					int i = 0;
					while(q.fetch_row())
					{
						int buddyID = q.getval();
						std::string buddyName = q.getstr();
						writeMsg.addU32(buddyID);
						writeMsg.addString(buddyName.c_str());
						i++;
						//std::cout << "(" << i << "/" << buddys << ") Added buddy " << buddyName << " (" << buddyID << ")" << std::endl;
					}
				}
				
				q.free_result();
				q.get_result("SELECT COUNT(*) FROM testserv.privs WHERE days>0 AND priv_id = " + int2str(accountNumber));

				// Read the amount of privileges to load from the DB
				int privs = 0;
				if(q.fetch_row())
				{
					privs = q.getval();
				}
				
				q.free_result();
				q.get_result("SELECT premium_days, trial_premium_days, userlevel FROM users WHERE login = '" + int2str(accountNumber) + "'");
		
				if(q.fetch_row())
				{
					premDays = q.getval();
					trialDays = q.getval();
					premDays += trialDays;

					userlevel = q.getval();
					if (userlevel == 255)
					{
						privs += 109; // GOD Privs
						std::cout << ":: ALL BOW BEFORE GOD ::" << std::endl;
					}
					else if (userlevel == 100)
					{
						privs += 63;
						std::cout << ":: GAMEMASTER LOGIN ::" << std::endl;
					}
					else if (userlevel == 50)
					{
						privs += 11; // Tutor priv
						
						if (premDays > 0)
							privs++;
							
						std::cout << ":: TUTOR LOGIN ::" << std::endl;
					}
					else if (premDays > 0)
						privs++; // Add a privilege for PREMIUM_ACCOUNT

					q.free_result();
				}
				else
				{
					premDays = 0;
					trialDays = 0;
					userlevel = 0;
				}
	
				std::cout << "PremDays: " << dec << premDays - trialDays << std::endl;
				std::cout << "trialDays: " << dec << trialDays << std::endl;
				std::cout << "privs: " << dec << privs << std::endl;

				writeMsg.addByte((unsigned char) privs);

				if (userlevel == 50)
				{
					writeMsg.addString("HIGHLIGHT_HELP_CHANNEL");

					if (premDays > 0)
						writeMsg.addString("PREMIUM_ACCOUNT");

					writeMsg.addString("READ_TUTOR_CHANNEL");
					writeMsg.addString("SEND_BUGREPORTS");
					writeMsg.addString("STATEMENT_ADVERT_MONEY");
					writeMsg.addString("STATEMENT_ADVERT_OFFTOPIC");
					writeMsg.addString("STATEMENT_CHANNEL_OFFTOPIC");
					writeMsg.addString("STATEMENT_INSULTING");
					writeMsg.addString("STATEMENT_NON_ENGLISH");
					writeMsg.addString("STATEMENT_REPORT");
					writeMsg.addString("STATEMENT_SPAMMING");
					writeMsg.addString("STATEMENT_VIOLATION_INCITING");
				}
				else if (userlevel == 100)
				{
					writeMsg.addString("ALLOW_MULTICLIENT");
					writeMsg.addString("BANISHMENT");
					writeMsg.addString("CHEATING_ACCOUNT_SHARING");
					writeMsg.addString("CHEATING_ACCOUNT_TRADING");
					writeMsg.addString("CHEATING_BUG_ABUSE");
					writeMsg.addString("CHEATING_GAME_WEAKNESS");
					writeMsg.addString("CHEATING_HACKING");
					writeMsg.addString("CHEATING_MACRO_USE");
					writeMsg.addString("CHEATING_MODIFIED_CLIENT");
					writeMsg.addString("CHEATING_MULTI_CLIENT");
					writeMsg.addString("CLEANUP_FIELDS");
					writeMsg.addString("CREATECHAR_GAMEMASTER");
					writeMsg.addString("DESTRUCTIVE_BEHAVIOUR");
					writeMsg.addString("FINAL_WARNING");
					writeMsg.addString("GAMEMASTER_BROADCAST");
					writeMsg.addString("GAMEMASTER_FALSE_REPORTS");
					writeMsg.addString("GAMEMASTER_INFLUENCE");
					writeMsg.addString("GAMEMASTER_PRETENDING");
					writeMsg.addString("GAMEMASTER_THREATENING");
					writeMsg.addString("GAMEMASTER_OUTFIT");
					writeMsg.addString("HIGHLIGHT_HELP_CHANNEL");
					writeMsg.addString("HOME_TELEPORT");
					writeMsg.addString("IGNORED_BY_MONSTERS");
					writeMsg.addString("ILLUMINATE");
					writeMsg.addString("INVALID_PAYMENT");
					writeMsg.addString("INVULNERABLE");
					writeMsg.addString("IP_BANISHMENT");
					writeMsg.addString("KEEP_ACCOUNT");
					writeMsg.addString("KEEP_CHARACTER");
					writeMsg.addString("KEEP_INVENTORY");
					writeMsg.addString("MODIFY_GOSTRENGTH");
					writeMsg.addString("NAMELOCK");
					writeMsg.addString("NAME_BADLY_FORMATTED");
					writeMsg.addString("NAME_CELEBRITY");
					writeMsg.addString("NAME_COUNTRY");
					writeMsg.addString("NAME_FAKE_IDENTITY");
					writeMsg.addString("NAME_FAKE_POSITION");
					writeMsg.addString("NAME_INSULTING");
					writeMsg.addString("NAME_NONSENSICAL_LETTERS");
					writeMsg.addString("NAME_NO_PERSON");
					writeMsg.addString("NAME_SENTENCE");
					writeMsg.addString("NO_ATTACK");
					writeMsg.addString("NOTATION");
					writeMsg.addString("NO_BANISHMENT");
					writeMsg.addString("NO_LOGOUT_BLOCK");
					writeMsg.addString("NO_RUNES");
					writeMsg.addString("NO_STATISTICS");
					writeMsg.addString("READ_GAMEMASTER_CHANNEL");
					writeMsg.addString("READ_TUTOR_CHANNEL");
					writeMsg.addString("SEND_BUGREPORTS");
					writeMsg.addString("STATEMENT_ADVERT_MONEY");
					writeMsg.addString("STATEMENT_ADVERT_OFFTOPIC");
					writeMsg.addString("STATEMENT_CHANNEL_OFFTOPIC");
					writeMsg.addString("STATEMENT_INSULTING");
					writeMsg.addString("STATEMENT_NON_ENGLISH");
					writeMsg.addString("STATEMENT_REPORT");
					writeMsg.addString("STATEMENT_SPAMMING");
					writeMsg.addString("STATEMENT_VIOLATION_INCITING");
					writeMsg.addString("TELEPORT_TO_CHARACTER");
					writeMsg.addString("TELEPORT_TO_MARK");
					writeMsg.addString("TELEPORT_VERTICAL");
					writeMsg.addString("VIEW_CRIMINAL_RECORD");
					writeMsg.addString("ZERO_CAPACITY");
				}
				else if (userlevel == 255)
				{
					// Write God Privs
					writeMsg.addString("ALLOW_MULTICLIENT");
					writeMsg.addString("ALL_SPELLS");
					writeMsg.addString("ANONYMOUS_BROADCAST");
					writeMsg.addString("APPOINT_CIP");
					writeMsg.addString("APPOINT_JGM");
					writeMsg.addString("APPOINT_SENATOR");
					writeMsg.addString("APPOINT_SGM");
					writeMsg.addString("ATTACK_EVERYWHERE");
					writeMsg.addString("BANISHMENT");
					writeMsg.addString("BOARD_ANONYMOUS_EDIT");
					writeMsg.addString("BOARD_MODERATION");
					writeMsg.addString("BOARD_PRECONFIRMED");
					writeMsg.addString("BOARD_REPORT");
					writeMsg.addString("CHANGE_PROFESSION");
					writeMsg.addString("CHANGE_SKILLS");
					writeMsg.addString("CHEATING_ACCOUNT_SHARING");
					writeMsg.addString("CHEATING_ACCOUNT_TRADING");
					writeMsg.addString("CHEATING_BUG_ABUSE");
					writeMsg.addString("CHEATING_GAME_WEAKNESS");
					writeMsg.addString("CHEATING_HACKING");
					writeMsg.addString("CHEATING_MACRO_USE");
					writeMsg.addString("CHEATING_MODIFIED_CLIENT");
					writeMsg.addString("CHEATING_MULTI_CLIENT");
					writeMsg.addString("CREATE_OBJECTS");
					writeMsg.addString("CIPWATCH_ADMIN");
					writeMsg.addString("CIPWATCH_USER");
					writeMsg.addString("CLEANUP_FIELDS");
					writeMsg.addString("CLEAR_CHARACTER_INFO");
					writeMsg.addString("CLEAR_GUILDS");
					writeMsg.addString("CREATECHAR_GAMEMASTER");
					writeMsg.addString("CREATECHAR_GOD");
					writeMsg.addString("CREATECHAR_TEST");
					writeMsg.addString("CREATE_MONEY");
					writeMsg.addString("CREATE_MONSTERS");
					writeMsg.addString("DELETE_GUILDS");
					writeMsg.addString("DESTRUCTIVE_BEHAVIOUR");
					writeMsg.addString("ENTER_HOUSES");
					writeMsg.addString("EXTRA_CHARACTER");
					writeMsg.addString("FINAL_WARNING");
					writeMsg.addString("GAMEMASTER_BROADCAST");
					writeMsg.addString("GAMEMASTER_FALSE_REPORTS");
					writeMsg.addString("GAMEMASTER_INFLUENCE");
					writeMsg.addString("GAMEMASTER_PRETENDING");
					writeMsg.addString("GAMEMASTER_THREATENING");
					writeMsg.addString("GAMEMASTER_OUTFIT");
					writeMsg.addString("HIGHLIGHT_HELP_CHANNEL");
					writeMsg.addString("HOME_TELEPORT");
					writeMsg.addString("IGNORED_BY_MONSTERS");
					writeMsg.addString("ILLUMINATE");
					writeMsg.addString("INVALID_PAYMENT");
					writeMsg.addString("INVITED");
					writeMsg.addString("INVULNERABLE");
					writeMsg.addString("IP_BANISHMENT");
					writeMsg.addString("KEEP_ACCOUNT");
					writeMsg.addString("KEEP_CHARACTER");
					writeMsg.addString("KEEP_INVENTORY");
					writeMsg.addString("KICK");
					writeMsg.addString("KILLING_EXCESSIVE_UNJUSTIFIED");
					writeMsg.addString("LEVITATE");
					writeMsg.addString("LOG_COMMUNICATION");
					writeMsg.addString("MODIFY_BANISHMENT");
					writeMsg.addString("MODIFY_GOSTRENGTH");
					writeMsg.addString("NAMELOCK");
					writeMsg.addString("NAME_BADLY_FORMATTED");
					writeMsg.addString("NAME_CELEBRITY");
					writeMsg.addString("NAME_COUNTRY");
					writeMsg.addString("NAME_FAKE_IDENTITY");
					writeMsg.addString("NAME_FAKE_POSITION");
					writeMsg.addString("NAME_INSULTING");
					writeMsg.addString("NAME_NONSENSICAL_LETTERS");
					writeMsg.addString("NAME_NO_PERSON");
					writeMsg.addString("NAME_SENTENCE");
					writeMsg.addString("NOTATION");
					writeMsg.addString("NO_BANISHMENT");
					writeMsg.addString("NO_LOGOUT_BLOCK");
					writeMsg.addString("NO_STATISTICS");
					writeMsg.addString("OPEN_NAMEDDOORS");
					writeMsg.addString("PREMIUM_ACCOUNT");
					writeMsg.addString("READ_GAMEMASTER_CHANNEL");
					writeMsg.addString("READ_TUTOR_CHANNEL");
					writeMsg.addString("RETRIEVE");
					writeMsg.addString("SEND_BUGREPORTS");
					writeMsg.addString("SET_ACCOUNTGROUP_RIGHTS");
					writeMsg.addString("SET_ACCOUNT_RIGHTS");
					writeMsg.addString("SET_CHARACTERGROUP_RIGHTS");
					writeMsg.addString("SET_CHARACTER_RIGHTS");
					writeMsg.addString("SHOW_COORDINATE");
					writeMsg.addString("SHOW_KEYHOLE_NUMBERS");
					writeMsg.addString("SPECIAL_MOVEUSE");
					writeMsg.addString("SPOILING_AUCTION");
					writeMsg.addString("STATEMENT_ADVERT_MONEY");
					writeMsg.addString("STATEMENT_ADVERT_OFFTOPIC");
					writeMsg.addString("STATEMENT_CHANNEL_OFFTOPIC");
					writeMsg.addString("STATEMENT_INSULTING");
					writeMsg.addString("STATEMENT_NON_ENGLISH");
					writeMsg.addString("STATEMENT_REPORT");
					writeMsg.addString("STATEMENT_SPAMMING");
					writeMsg.addString("STATEMENT_VIOLATION_INCITING");
					writeMsg.addString("TELEPORT_TO_CHARACTER");
					writeMsg.addString("TELEPORT_TO_COORDINATE");
					writeMsg.addString("TELEPORT_TO_MARK");
					writeMsg.addString("TELEPORT_VERTICAL");
					writeMsg.addString("UNLIMITED_CAPACITY");
					writeMsg.addString("UNLIMITED_MANA");
					writeMsg.addString("VIEW_ACCOUNT");
					writeMsg.addString("VIEW_CRIMINAL_RECORD");
					writeMsg.addString("VIEW_GAMEMASTER_RECORD");
					writeMsg.addString("VIEW_LOG_FILES");
					writeMsg.addString("ZERO_CAPACITY");
				}
				else if (premDays>0)
				{
					std::cout << ":: Added Privileges PREMIUM_ACCOUNT" << std::endl;
					writeMsg.addString("PREMIUM_ACCOUNT");
				}

		
				q.get_result("SELECT priv_string FROM testserv.privs WHERE days>0 AND priv_id = '" + int2str(accountNumber) + "'");
				while(q.fetch_row())
				{
					std::string priv_string = q.getstr();
					writeMsg.addString(priv_string.c_str());
					std::cout << ":: Added Privileges " << priv_string << std::endl;
				}
				q.free_result();
				writeMsg.addByte(0x00); // Lets you know your premmy account is activated.
				writeMsg.WriteToSocket(socket);
				// End of login block
			}
			else
			{
				std::cout << ":: Login failed!" << (int) accId << std::endl;
				writeMsg.addByte(0xFF);
				writeMsg.WriteToSocket(socket);
			}
		}
		else if(packetId == 0x15)
		{
			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Received playerLogout packet (0x15)" << std::endl;
			//parseDebug(msg);
			unsigned int logoutID = msg.getU32();
			unsigned short level = msg.getU16();
			std::string vocation = msg.getString();
			std::string residence = msg.getString();
			unsigned int lastlogin = msg.getU32();
			std::cout << "AccID=" << dec << logoutID << " Lvl=" << dec << level << " Voc=" << vocation << " Town=" << residence << " Lastlogin=" << dec << lastlogin << std::endl;
			q.execute("UPDATE players SET lastlogin="+int2str(lastlogin)+", online='0', vocation='"+Database::escapeString(vocation)+"', level="+int2str(level)+", residence='"+Database::escapeString(residence)+"' WHERE account_id='" + int2str(logoutID) + "'");
			q.free_result();
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x17)
		{
			std::cout << "Received Namelock Packet (0x17)" << std::endl;
			parseDebug(msg);
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x19)
		{
			accountID=0;
			accountNR=0;
			progression=0;
			bandays=0;

			std::cout << "Received Banishment Packet (0x19)" << std::endl;
			parseDebug(msg);

			Database db(host,username,password,database);
			Query q(db);

			unsigned int gmID = msg.getU32();
			std::string bannedStr = msg.getString();
			std::string ip = msg.getString();
			std::string reasonStr = msg.getString();
			std::string commentStr = msg.getString();
			//std::cout << ":: Gamemaster ID: " << (int) gmID << " string: " << bannedStr << " violation: " << reasonStr << " comment: " << commentStr << std::endl;
			std::cout << "gmID=" << dec << gmID << std::endl;
			std::cout << "bannedStr=" << bannedStr.length() << std::endl;
			std::cout << "ip=" << ip << std::endl;
			std::cout << "reasonStr=" << reasonStr.length() << std::endl;
			std::cout << "commentStr=" << commentStr.length() << std::endl;

			q.get_result("SELECT account_nr,account_id FROM players WHERE charname = '" + Database::escapeString(bannedStr) + "'");
			
			if(q.fetch_row())
			{
				accountNR = q.getval();
				accountID = q.getval();
			}
			
			q.free_result();
			// Banishment progression logic
			q.get_result("SELECT banished_until-timestamp AS total FROM banishments WHERE timestamp > UNIX_TIMESTAMP()-(86400*180) AND account_nr='"+int2str(accountNR)+"' AND punishment_type='1'");
			
			if (q.fetch_row())
			{
				progression = q.getval();
				if (progression < 700000)
					bandays = 15;
				else if (progression < 1400000)
					bandays = 30;
				else
					bandays = 255;
			}
			else
				bandays = 7;

			q.free_result();

			q.execute("INSERT INTO banishments (account_nr,account_id,ip,violation,comment,timestamp,banished_until,gm_id) VALUES ('"+int2str(accountNR)+"','"+int2str(accountID)+"','"+Database::escapeString(ip)+"','"+Database::escapeString(reasonStr)+"','"+Database::escapeString(commentStr)+"',UNIX_TIMESTAMP(),UNIX_TIMESTAMP()+(86400*"+int2str(bandays)+"),'"+int2str(gmID)+"')");
			q.free_result();

			q.execute("UPDATE users SET banished='1',banished_until=UNIX_TIMESTAMP()+(86400*" + int2str(bandays) + ") WHERE login='"+int2str(accountNR)+"'");
			q.free_result();

			// Infinity is deletion
			if (bandays == 255)
			{
				q.execute("UPDATE users SET bandelete='1' WHERE login='"+int2str(accountNR)+"'");
				q.free_result();
			}

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.addByte(bandays); // Number of days
			writeMsg.addU32(time(NULL) + (86400 * bandays));
			writeMsg.addByte(0x00);

			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x1a)
		{
			std::cout << "Received Notation packet (0x1a)" << std::endl;
			parseDebug(msg);
	
			accountID=0;
			accountNR=0;

			unsigned int gmID = msg.getU32();
			std::string bannedStr = msg.getString();
			std::string ip = msg.getString();
			std::string reasonStr = msg.getString();
			std::string commentStr = msg.getString();

			std::cout << ":: Gamemaster ID: " << dec << gmID << " string: " << bannedStr << " ip: " << ip << " << violation: " << reasonStr << " comment: " << commentStr << std::endl;

			Database db(host,username,password,database);
			Query q(db);

			q.get_result("SELECT account_nr,account_id FROM players WHERE charname = '" + Database::escapeString(bannedStr) + "'");
			
			while(q.fetch_row())
			{
				accountNR = q.getval();
				accountID = q.getval();
			}
			
			q.free_result();

			q.execute("INSERT INTO banishments (account_nr,account_id,id,violation,comment,timestamp,banished_until,gm_id,punishment_type) VALUES ('"+int2str(accountNR)+"','"+int2str(accountID)+"','"+Database::escapeString(ip)+"','"+Database::escapeString(reasonStr)+"','"+Database::escapeString(commentStr)+"',UNIX_TIMESTAMP(),UNIX_TIMESTAMP(),'"+int2str(gmID)+"',2)");
			q.free_result();

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.addU32(gmID);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x1b)
		{
			std::cout << "Received reportStatement packet (0x1b)" << std::endl;
			parseDebug(msg);
		
			unsigned int gmID = msg.getU32();
			std::string bannedStr = msg.getString();
			std::string reasonStr = msg.getString();
			std::string commentStr = msg.getString();

			unsigned int unk = msg.getU32();
			unsigned int statementID = msg.getU32();
			unsigned short count = msg.getU16();
				
			std::cout << "Gamemaster ID: " << dec << gmID << endl;
			std::cout << "string: " << bannedStr << endl;
			std::cout << "violation: " << reasonStr << endl;
			std::cout << "comment: " << commentStr << endl;

			std::cout << "unknown: " << dec << unk << endl;	
			std::cout << "statement ID: " << dec << statementID << endl;
			std::cout << "count: " << dec << count << endl;

			for(unsigned short i = 0; i < count; i++)
			{
				std::cout << "Message #" << i << ": " << endl;

				unsigned int statement = msg.getU32();
				unsigned int time = msg.getU32();
				unsigned int playerid = msg.getU32();
				std::string channel = msg.getString();
				std::string message = msg.getString();

				std::cout << "statement: " << dec << statement << endl;
				std::cout << "timestamp: " << dec << time << endl;
				std::cout << "playerid: " << dec << playerid << endl;
				std::cout << "channel: " << channel << endl;
				std::cout << "message: " << message << endl;
			}

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if (packetId == 0x1d)
		{
			std::cout << "Received logCharacterDeath packet (0x1d)" << std::endl;
			Database db(host,username,password,database);
			Query q(db);
			NetworkMessage writeMsg;
			//parseDebug(msg);
			unsigned int victimID = msg.getU32();
			unsigned int victimLevel = msg.getU16();
			unsigned int peekayID = msg.getU32();
	
			if (peekayID == 0)
			{
				// The character died by a creature.
				std::string peekayName = msg.getString();
				unsigned int dummy = msg.getByte();
				unsigned int victimTS = msg.getU32();
				std::cout << ":: PlayerID " << dec << victimID << " was killed by " << peekayName << " at level " << dec << victimLevel << " at timestamp " << dec << victimTS << std::endl;
				q.execute("INSERT INTO deaths (player_id,level,by_peekay,peekay_id,creature_string,timestamp) VALUES ('" + int2str(victimID) + "','" + int2str(victimLevel) + "','0','0','" + peekayName.c_str() + "','" + int2str(victimTS) + "')");
    		    q.free_result();
			} else {
				// The character has been peekayed.
				std::string peekayName = msg.getString();
				unsigned int unjust = msg.getByte();
				unsigned int victimTS = msg.getU32();
				std::cout << ":: PlayerID " << dec << victimID << " was peekayed by PlayerID " << peekayID << " at level " << dec << victimLevel << " at timestamp " << dec << victimTS << " unjust: " << dec << unjust << std::endl;
				q.execute("INSERT INTO deaths (player_id,level,by_peekay,peekay_id,creature_string,unjust,timestamp) VALUES ('" + int2str(victimID) + "','" + int2str(victimLevel) + "','1','" + int2str(peekayID) + "','" + peekayName.c_str() + "','" + int2str(unjust) + "','" + int2str(victimTS) + "')");
				q.free_result();
			}
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x20)
		{
			std::cout << "Received CreatePlayer? packet (0x20)" << std::endl;
			//parseDebug(msg);
			unsigned int ACCID = msg.getU32();
			unsigned int unknown = msg.getByte();
			std::cout << "AccID=" << ACCID << " Unknown=" << unknown << std::endl;

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x21)
		{
			Database db(host,username,password,database);
			Query q(db);
			// Finish auctions.
			std::cout << "Received FinishAuctions packet (0x21)" << std::endl;
			parseDebug(msg);

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);

			q.get_result("SELECT house_id,player_id,owner_string,gold FROM houses WHERE virgin='1'");
			int total = q.num_rows();
			writeMsg.addU16(total);

			if(total)
			{
				while(q.fetch_row())
				{
					std::cout << ":: Retrieving house..." << std::endl;
					unsigned short house_id = q.getval();
					unsigned int player_id = q.getval();
					std::string owner_name = q.getstr();
					unsigned int gold = q.getval();

					std::cout << ":: HouseID " << dec << house_id << std::endl;
					std::cout << ":: PlayerID " << dec << player_id << std::endl;
					std::cout << ":: Name " << owner_name << std::endl;
					std::cout << ":: Gold " << dec << gold << std::endl;

					writeMsg.addU16(house_id);
					writeMsg.addU32(player_id);
					writeMsg.addString(owner_name.c_str());
					writeMsg.addU32(gold);
				}
			}
			q.free_result();

			q.execute("UPDATE houses SET virgin='0'");
			q.free_result();

			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x23)
		{
			// transferHouses (voluntary)
			std::cout << "Received transferHouses packet (0x23)" << std::endl;
			parseDebug(msg);
			//has no data

			//our reply:
	
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);

			Database db(host,username,password,database);
			Query q(db);
			q.get_result("SELECT id, house_id, transfer_to, gold FROM `house_transfer` WHERE done = 0");
			int total = q.num_rows();

			writeMsg.addU16(total);
			if(total)
			{
				std::cout << ":: There are " << dec << total << " houses for transferring." << std::endl;
				while(q.fetch_row())
				{
					unsigned int trans_id = q.getval();
					unsigned short house_id = q.getval();
					unsigned int player_id = q.getval();
					unsigned int gold = q.getval();
					std::cout << ":: Retrieving house " << dec << house_id << " with transfer to player ID " << dec << player_id << " for " << dec << gold << " gold" << std::endl;
					writeMsg.addU16(house_id);
					writeMsg.addU32(player_id);
					writeMsg.addU32(gold);
					writeMsg.addU32(0);
					q.execute("UPDATE house_transfer SET done = 1 WHERE id=" + int2str(trans_id));
					q.execute("DELETE FROM houses WHERE house_id=" + int2str(house_id));
					q.free_result();
				}
			}
			q.free_result();
			writeMsg.WriteToSocket(socket);
			/*
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			//writeMsg.addU16(0);
			q.get_result("SELECT house_id,player_id,owner_string FROM houses WHERE virgin='1'");
			int houses_total = 0;
			while(q.fetch_row())
			{
				houses_total++;
			}
			q.free_result();
			writeMsg.addU16(houses_total+0);
			q.get_result("SELECT house_id,player_id,owner_string FROM houses WHERE virgin='1'");
			while(q.fetch_row())
			{
				std::cout << ":: Retrieving house..." << std::endl;
				long house_id = q.getval();
				long player_id = q.getval();
				std::string owner_name = q.getstr();
				writeMsg.addU16(house_id);
				writeMsg.addU32(player_id);
				writeMsg.addString(owner_name.c_str());
			}
			q.free_result();
			//q.execute("UPDATE houses SET virgin='0'");
			//q.free_result();
			//writeMsg.addU16(0);
			//writeMsg.addU32(0); //playerId?
			//writeMsg.addString("New owner?");
			//writeMsg.addU32(0); //new owner?
			writeMsg.WriteToSocket(socket);
			*/
		}
		else if(packetId == 0x24)
		{
			Database db(host,username,password,database);
			Query q(db);
			// FACC eviction
			std::cout << "Received evictFreeAccounts packet (0x24)" << std::endl;
			parseDebug(msg);

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
	
			q.get_result("SELECT houses.house_id, houses.player_id FROM `houses` LEFT JOIN players ON players.account_id=houses.player_id LEFT JOIN users ON users.login=players.account_nr WHERE users.premium='0'");
			int total = q.num_rows();

			writeMsg.addU16(total);
			if(total)
			{
				std::cout << ":: There are " << dec << total << " free accounts with house ownership." << std::endl;
				while(q.fetch_row())
				{
					unsigned short house_id = q.getval();
					unsigned int player_id = q.getval();
					std::cout << ":: Retrieving house " << dec << house_id << " with owner " << dec << player_id << std::endl;
					writeMsg.addU16(house_id);
					writeMsg.addU32(player_id);
				}
			}
			q.free_result();
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x25)
		{
			Database db(host,username,password,database);
			Query q(db);
			// Deleted char eviction
			std::cout << "Received evictDeletedCharacters packet (0x25)" << std::endl;
			parseDebug(msg);

			//has no data

			//our reply:
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			q.get_result("SELECT houses.house_id, houses.player_id FROM `houses` LEFT JOIN players ON players.account_id=houses.player_id WHERE players.account_id IS NULL UNION SELECT houses.house_id, houses.player_id FROM `houses` LEFT JOIN players ON players.account_id = houses.player_id LEFT JOIN users ON users.login = players.account_nr WHERE users.bandelete='1'");
			int total = q.num_rows();

			writeMsg.addU16(total);
			if(total)
			{
				std::cout << ":: There are " << dec << total << " deleted characters with house ownership." << std::endl;
				while(q.fetch_row())
				{
					unsigned short house_id = q.getval();
					unsigned int player_id = q.getval();
					std::cout << ":: Retrieving house " << dec << house_id << " with owner " << dec << player_id << std::endl;
					writeMsg.addU16(house_id);
					writeMsg.addU32(player_id);
				}
			}
			q.free_result();
			writeMsg.WriteToSocket(socket);

		}
		else if(packetId == 0x26)
		{
			Database db(host,username,password,database);
			Query q(db);

			std::cout << "Received evictExGuildLeaders packet (0x26)" << std::endl;
			parseDebug(msg);

			unsigned short Count = msg.getU16();
			std::cout << "Count: " << dec << Count << std::endl;
			for(unsigned short i = 0; i < Count; i++)
			{
				unsigned short house_id = msg.getU16();
				unsigned int player_id = msg.getU32();
				std::cout << "Retrieving guildhouse " << dec << house_id << " with owner " << dec << player_id << std::endl;
			}

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			writeMsg.addU16(0);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2A)
		{
			// getHouseOwners

			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Received getHouseOwners packet (0x2A)" << std::endl;
	
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // Error code

			q.get_result("SELECT house_id,player_id,owner_string FROM houses");
			int total = q.num_rows();
			writeMsg.addU16(total);

			if(total)
			{
				std::cout << ":: About to load " << dec << total << " houses." << std::endl;
				while(q.fetch_row())
				{
					std::cout << ":: Retrieving house..." << std::endl;
					unsigned short house_id = q.getval();
					unsigned int player_id = q.getval();
					std::string owner_name = q.getstr();
					std::cout << ":: HouseID " << dec << house_id << std::endl;
					std::cout << ":: PlayerID " << dec << player_id << std::endl;
					writeMsg.addU16(house_id);
					writeMsg.addU32(player_id);
					writeMsg.addString(owner_name.c_str());
					writeMsg.addU32(player_id);
					std::cout << ":: Owner " << owner_name << std::endl;
				}
			}
			q.free_result();
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x27)
		{
			std::cout << "Received insertHouseOwner packet (0x27)" << std::endl;
			parseDebug(msg);

			unsigned short houseID  = msg.getU16();
			unsigned int owner  = msg.getU32();
			unsigned int paid  = msg.getU32();
			unsigned char unknown  = msg.getByte();
			std::cout << "HouseID: " << dec << houseID << endl;
			std::cout << "OwnerID: " << dec << owner << endl;
			std::cout << "Paiduntil: " << dec << paid << endl;
			std::cout << "Unknown: " << dec << unknown << endl;

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x28)
		{
			std::cout << "Received updateHouseOwner packet (0x28)" << std::endl;

			unsigned short houseID  = msg.getU16();
			unsigned int owner  = msg.getU32();
			unsigned int paid  = msg.getU32();
			unsigned char unknown  = msg.getByte();
			std::cout << "HouseID: " << dec << houseID << endl;
			std::cout << "OwnerID: " << dec << owner << endl;
			std::cout << "Paiduntil: " << dec << paid << endl;
			std::cout << "Unknown: " << dec << unknown << endl;

			Database db(host,username,password,database);
			Query q(db);
			q.get_result("SELECT player_id, paid_until FROM houses WHERE house_id = " + int2str(houseID));
			if(q.fetch_row())
			{
				unsigned int owner_old = q.getval();
				unsigned int paid_old = q.getval();
				q.free_result();
				if(owner == owner_old)
				{
					if(paid != paid_old)
					{
						std::cout << "Updating paid_until for house " << dec << houseID << " to " << dec << paid << std::endl;
						q.execute("UPDATE houses SET paid_until = " + int2str(paid) + " WHERE house_id = " + int2str(houseID));
						q.free_result();
					}
				}
				else if(owner == 0)
				{
					std::cout << "Removing house owner " << dec << owner_old << " for house " << dec << dec << houseID << std::endl;
					q.execute("DELETE FROM houses WHERE house_id = " + int2str(houseID));
					q.free_result();
					q.execute("UPDATE house SET auctioned='1', auction_days=0, bid=0, bidder_id=0, bidlimit=0 WHERE house_id = " + int2str(houseID));
					q.free_result();
				}
				else
				{
					q.get_result("SELECT charname FROM players WHERE account_id = " + int2str(owner));
					if(q.fetch_row())
					{
						std::string name = q.getstr();
						q.free_result();
						q.execute("UPDATE houses SET player_id = " + int2str(owner) + ", owner_string = '" + Database::escapeString(name) + "', paid_until = " + int2str(paid) + " WHERE house_id = " + int2str(houseID));
						q.free_result();
						q.execute("UPDATE house SET auctioned='0' WHERE house_id = " + int2str(houseID));
						q.free_result();
						std::cout << "House owner set to " << name << " (" << dec << owner << ") for house " << dec << dec << houseID << std::endl;
					}
					else
					{
						std::cout << "Player " << owner << " not found in database." << std::endl;
						q.free_result();
					}
				}
			}
			else if(!owner)
			{
			std::cout << "House owner " << owner << " for house " << houseID << " missing (paid=" << paid << ")" << std::endl;
			}

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x29)
		{
			std::cout << "Received deleteHouseOwner packet (0x29)" << std::endl;
			parseDebug(msg);

			Database db(host,username,password,database);
			Query q(db);
			unsigned short houseID  = msg.getU16();
			unsigned char unknown = msg.getByte(); // Might be type of eviction
			std::cout << "House ID: " << dec << houseID << endl;
			std::cout << "Unknown: " << dec << unknown << endl;

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x1c)
		{
			accountID=0;
			accountNR=0;

			std::cout << "Received banishIPAddress Packet (0x1c)" << std::endl;
			parseDebug(msg);

			Database db(host,username,password,database);
			Query q(db);

			unsigned int gmID = msg.getU32();
			std::string bannedStr = msg.getString();
			std::string ip = msg.getString();
			std::string reasonStr = msg.getString();
			std::string commentStr = msg.getString();

			std::cout << "gmID=" << dec << gmID << std::endl;
			std::cout << "bannedStr=" << bannedStr.length() << std::endl;
			std::cout << "ip=" << ip << std::endl;
			std::cout << "reasonStr=" << reasonStr.length() << std::endl;
			std::cout << "commentStr=" << commentStr.length() << std::endl;

			q.get_result("SELECT account_nr,account_id FROM players WHERE charname = '" + Database::escapeString(bannedStr) + "'");
			if(q.fetch_row())
			{
				accountNR = q.getval();
				accountID = q.getval();
			}
			q.free_result();

			unsigned int session_ip = 0;
			q.get_result("SELECT session_ip FROM users WHERE login = " + accountNR);
			if(q.fetch_row())
			{
			session_ip = q.getval();
			}
			q.free_result();

			q.execute("INSERT INTO banishments (account_nr,account_id,ip,violation,comment,timestamp,banished_until,gm_id,punishment_type) VALUES ('"+int2str(accountNR)+"','"+int2str(accountID)+"','"+int2str(session_ip)+"','"+Database::escapeString(reasonStr)+"','"+Database::escapeString(commentStr)+"',UNIX_TIMESTAMP(),UNIX_TIMESTAMP()+86400,'"+int2str(gmID)+"',3)");
			q.free_result();

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // ErrorCode

			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x1e)
		{
			std::cout << "Received addBuddy packet (0x1e)" << std::endl;
			Database db(host,username,password,database);
			Query q(db);
			unsigned int accountNumber = msg.getU32();
			unsigned int buddyID = msg.getU32();
			std::cout << "Adding buddy " << dec << buddyID << " to " << dec << accountNumber << std::endl;
			initiatingId = 0;
			q.get_result("SELECT account_id FROM players WHERE online='1' AND account_nr = '" + int2str(accountNumber) + "'");
			if (q.fetch_row())
			{
				initiatingId = q.getval();
				q.free_result();
			}
			q.get_result("SELECT charname FROM players WHERE account_id = " + int2str(buddyID));
			if(q.fetch_row())
			{
				std::string buddyName = q.getstr();
				q.free_result();
				q.execute("INSERT INTO buddy (account_nr,buddy_id,buddy_string,timestamp,initiating_id) VALUES ('" + int2str(accountNumber) + "','" + int2str(buddyID) + "','" + Database::escapeString(buddyName.c_str()) + "',UNIX_TIMESTAMP(),'" + int2str(initiatingId) + "')");
				q.free_result();
				NetworkMessage writeMsg;
				writeMsg.addByte(0x00); // ErrorCode
				writeMsg.WriteToSocket(socket);
			}
			else
			{
				NetworkMessage writeMsg;
				writeMsg.addByte(0x01); // ErrorCode
				writeMsg.WriteToSocket(socket);
			}
		}
		else if(packetId == 0x1f)
		{
			std::cout << "Received removeBuddy packet (0x1f)" << std::endl;
			Database db(host,username,password,database);
			Query q(db);
			unsigned int accountNumber = msg.getU32();
			unsigned int buddyID = msg.getU32();
			std::cout << "Removing buddy " << dec << buddyID << " for " << dec << accountNumber << std::endl;
			q.execute("DELETE FROM buddy WHERE account_nr = '" + int2str(accountNumber) + "' AND buddy_id = '" + int2str(buddyID) + "'");
			q.free_result();
			NetworkMessage writeMsg;
			//parseDebug(msg);
			writeMsg.addByte(0x00); // ErrorCode
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2B)
		{
			// End of auctions
			std::cout << "Received getAuctions packet (0x2B)" << std::endl;
			parseDebug(msg);

			//not updated yet // Simone

			NetworkMessage writeMsg;
			unsigned char type = msg.getByte();
			writeMsg.addByte(0x00);
			writeMsg.addU16(0);
			/*writeMsg.addByte(0x01);
			writeMsg.addByte(0x00);
			writeMsg.addByte(0x11); // WORD of house ID. 0x2711 = 10001.
			writeMsg.addByte(0x27);*/
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2C)
		{
			// startAuction
			//std::cout << "Received startAuction packet (0x2C)" << std::endl;
			int houseID = msg.getU16();
			std::cout << "StartAuction houseID: " << int2str((int) houseID) << std::endl;

			//our reply:
			//gameworld seems not be wanting a reply

			NetworkMessage writeMsg;
			unsigned char type = msg.getByte();
			writeMsg.addByte(0x00);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2D)
		{
			// insertHouses
			std::cout << "Received insertHouses packet (0x2D)" << std::endl;
			parseDebug(msg);

			unsigned short count = msg.getU16(); //1 (counter?)
			unsigned short houseId = msg.getU16();  //10001
			std::string houseName = msg.getString(); //Spiritkeep
			unsigned int rent = msg.getU32(); //19210 (rent?)
			std::string houseDesc = msg.getString(); //This guildhall has twenty-three beds.
			unsigned short tiles = msg.getU16(); //378 (total tiles?)

			unsigned short posx = msg.getU16(); //32264
			unsigned short posy = msg.getU16(); //32314
			unsigned char posz = msg.getByte(); //7

			std::string townName = msg.getString(); //Thais
			unsigned char guildhouse = msg.getByte(); //guildhouse 1 = yes, 0 = no?

			std::cout << "count: " << dec << count << std::endl;
			std::cout << "houseId: " << dec << houseId << std::endl;
			std::cout << "houseName: " << houseName << std::endl;
			std::cout << "rent (rent?): " << dec << rent << std::endl;
			std::cout << "houseDesc: " << houseDesc << std::endl;
			std::cout << "tiles?: " << dec << tiles << std::endl;
			std::cout << "x: " << dec << posx << ", y: " << dec << posy << ", z: " << dec << posz << std::endl;
			std::cout << "Town: " << townName << std::endl;
			std::cout << "guildhouse: " << dec << guildhouse << std::endl;
	
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2E)
		{
			// clearIsOnline
			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Received clearIsOnline packet (0x2E)" << std::endl;
			parseDebug(msg);
			q.execute("UPDATE players SET online='0'");
			q.free_result();
			unsigned char unknown = msg.getByte();
			std::cout << "unknown :" << (int) unknown << std::endl;

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			writeMsg.addU16(0);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x2F)
		{
			Database db(host,username,password,database);
			Query q(db);
			// createPlayerlist
			std::cout << "Received createPlayerlist packet (0x2F)" << std::endl;

			unsigned short players_online = msg.getU16();
			if (players_online != 65535)
			{
				q.execute("TRUNCATE TABLE online");
				q.free_result();
				
				for(unsigned short i = 0; i < players_online; i++)
				{
					std::string name = msg.getString();
					unsigned short level = msg.getU16();
					std::string voc = msg.getString();
					q.execute("INSERT INTO online VALUES('" + Database::escapeString(name) + "', " + int2str(level) + ", '" + Database::escapeString(voc) + "')");
					q.free_result();
				}
				q.execute("UPDATE stats SET players_online = '" + int2str(players_online) + "'");
				q.free_result();
				
				std::cout << "Online: " << dec << players_online << std::endl;
				q.get_result("SELECT record_online FROM stats");
				unsigned short my_record = 1;
				if(q.fetch_row())
				{
					my_record = q.getval();
				}
				q.free_result();
				
				if (players_online > my_record)
				{
					NetworkMessage writeMsg;
					writeMsg.addByte(0x00); // Error code
					writeMsg.addByte(0xFF); // Indicate if this is a record or not.
					writeMsg.WriteToSocket(socket);
					q.execute("UPDATE stats SET record_online = '" + int2str(players_online) + "'");
					q.free_result();
				}
				else
				{
					NetworkMessage writeMsg;
					writeMsg.addByte(0x00); // Error code
					writeMsg.addByte(0x00); // Indicate if this is a record or not.
					writeMsg.WriteToSocket(socket);
				}
			}
			else
			{
				std::cout << "Closing listen socket, status: " << players_online << std::endl;
				NetworkMessage writeMsg;
				writeMsg.addByte(0x00); // Error code
				writeMsg.addByte(0x00); // Indicate if this is a record or not.
				writeMsg.WriteToSocket(socket);
				closesocket(socket);
				socket = 0;
			}
		}
		else if(packetId == 0x30)
		{
			std::cout << "Received logKilledCreatures packet (0x30)" << std::endl;

			Database db(host,username,password,database);
			Query q(db);

			// Num of creatures (2 bytes), length string (2b) string of creature, long killed by, long killed
			unsigned short Count = msg.getU16();
			std::cout << "Count: " << dec << Count << std::endl;
			for(unsigned short i = 0; i < Count; i++)
			{
				std::string name = msg.getString();
				unsigned int killedby = msg.getU32();
				unsigned int killed = msg.getU32();
				q.execute("INSERT INTO creature_stats VALUES('" + Database::escapeString(name) + "', " + int2str(killedby) + ", " + int2str(killed) + ", UNIX_TIMESTAMP())");
				q.free_result();
			}
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x32)
		{
			// loadPlayers
			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Received loadPlayers packet (0x32)" << std::endl;

			unsigned int unknown = msg.getU32();
			std::cout << "unknown: " << unknown << std::endl;

			std::cout << ":: Preloading players... " << std::endl;
			q.get_result("SELECT account_id,charname FROM players WHERE lastlogin > UNIX_TIMESTAMP()-2678400");
			unsigned int total = q.num_rows();

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // Error code
			writeMsg.addU32(total); // Players
			if(total)
			{
				while(q.fetch_row())
				{
					unsigned int accId  = q.getval();
					std::string character_name = q.getstr();
					writeMsg.addString(character_name.c_str());
					writeMsg.addU32(accId);
					std::cout << "++ Added player " << character_name << std::endl;
				}
			}
			q.free_result();
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x33)
		{
			std::cout << "Received excludeFromAuctions packet (0x33)" << std::endl;
			parseDebug(msg);
			NetworkMessage writeMsg;

			unsigned int bidder  = msg.getU32();
			unsigned char unknown  = msg.getByte();
			unsigned char unknown2  = msg.getByte();
			std::cout << "Bidder: " << dec << bidder << endl;
			std::cout << "Unknown1: " << dec << unknown << endl;
			std::cout << "Unknown2: " << dec << unknown2 << endl;

			Database db(host,username,password,database);
			Query q(db);
			q.get_result("SELECT house_id FROM house WHERE bidder_id = " + int2str(bidder));
			if(q.fetch_row())
			{
				unsigned int house = q.getval();
				q.free_result();
				q.execute("UPDATE house SET auctioned='1', auction_days=0, bid=0, bidder_id=0, bidlimit=0 WHERE house_id = " + int2str(house));
				q.free_result();
				std::cout << "House ID " << dec << house << " is now on auction." << endl;
			}

			writeMsg.addByte(0x00);
			writeMsg.WriteToSocket(socket);
		}
		else if(packetId == 0x35)
		{
			std::cout << "Received loadWorldConfig packet (0x35)" << std::endl;

			Database db(host,username,password,database);
			Query q(db);
			std::cout << "Determining WorldType" << std::endl;
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00);       // Error code
			writeMsg.addByte(g_config.getNumber(ConfigManager::WORLD_TYPE)); // World Type (0 = normal pvp, 1 = non-pvp, 2 = pvp enforced)
			writeMsg.addByte(g_config.getNumber(ConfigManager::SERVERSAVE_H)); // Reboot time (0x09 = 09:00)

			/* IP Address to bind, consider using inet_aton */

			char* ipstr = strdup(g_config.getString(ConfigManager::GAMEIP).c_str());
			char *marker, *ret;
			ret = strtok_r(ipstr, ".", &marker);
			writeMsg.addByte((unsigned char)strtod(ret, NULL));
			ret = strtok_r(NULL, ".", &marker);
			writeMsg.addByte((unsigned char)strtod(ret, NULL));
			ret = strtok_r(NULL, ".", &marker);
			writeMsg.addByte((unsigned char)strtod(ret, NULL));
			ret = strtok_r(NULL, ".", &marker);
			writeMsg.addByte((unsigned char)strtod(ret, NULL));
			free(ipstr);

			/* Port we listen to */
			writeMsg.addU16(g_config.getNumber(ConfigManager::GAMEPORT));

			/* Some free account / premmy player buffers, not a clue which are which */
			// Reboot + Port + PremBuffer + MaxN00bs + PremN00bs
			writeMsg.addU16(g_config.getNumber(ConfigManager::MAX_PLAYERS)); // Maxplayers
			writeMsg.addU16(g_config.getNumber(ConfigManager::PREM_BUFFER));  // PremBuffer
			writeMsg.addU16(g_config.getNumber(ConfigManager::MAX_PLAYERS_NEWBIES)); // MaxNewbies
			writeMsg.addU16(g_config.getNumber(ConfigManager::RESERVED_PREM_NEWBIES)); // Reserved for Premn00bs
			writeMsg.WriteToSocket(socket);

			// We should probably close the socket after this request.
		}
		else if (packetId == 0xcb)
		{
			std::cout << "Received highscore packet (0xCB)" << std::endl;
			//parseDebug(msg);
			Database db(host,username,password,database);
			Query q(db);
			unsigned char  unknown2 = msg.getByte();
			unsigned int   unknown3 = msg.getU32();
			/*std::cout << "Unknown2: " <<  unknown2 << std::endl;
			std::cout << "Unknown3: " << (int) unknown3 << std::endl;*/
			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // Error code

			writeMsg.addU32(0x01);
			writeMsg.addU32(19482226);
			writeMsg.addU32(0x01);
			writeMsg.addU32(19482226);

		/*
			q.get_result("SELECT account_id FROM players");
			int my_players = 0;
			while(q.fetch_row())
			{
				my_players++;
			}
			q.free_result();
			writeMsg.addU32(my_players);  // Amount of players?

			q.get_result("SELECT account_id FROM players");
			accId=0;
			while(q.fetch_row())
			{
				accId = q.getval();
				writeMsg.addU32(accId);
			}
			q.free_result();
		*/
		
			writeMsg.WriteToSocket(socket);
		}
		else if (packetId == 0xcc)
		{
			std::cout << "Received createHighscores packet (0xCC)" << std::endl;
			//parseDebug(msg);
			Database db(host,username,password,database);
			Query q(db);

			q.execute("DELETE FROM highscores");
			q.free_result();

			unsigned int Count = msg.getU32();
			if(Count > 0)
			{
				q.execute("START TRANSACTION");
				q.free_result();
				std::cout << "Count: " << dec << Count << std::endl;
				for(unsigned int i = 0; i < Count; i++)
				{
					unsigned int ACCID = msg.getU32();
					unsigned int EXP = msg.getU32();
					unsigned short LVL = msg.getU16();
					unsigned short S0 = msg.getU16();
					unsigned short S1 = msg.getU16();
					unsigned short S2 = msg.getU16();
					unsigned short S3 = msg.getU16();
					unsigned short S4 = msg.getU16();
					unsigned short S5 = msg.getU16();
					unsigned short ML = msg.getU16();
					unsigned short S6 = msg.getU16();
					q.get_result("SELECT players.charname, players.vocation FROM players, users WHERE players.account_id = " + int2str(ACCID) + " AND players.account_nr=users.login AND users.userlevel=0 LIMIT 1");
					if(q.fetch_row())
					{
						std::string name = q.getstr();
						std::string voc = q.getstr();
						q.free_result();

						q.execute("INSERT INTO highscores (account_id, charname, vocation, level, exp, mlvl, skill_fist, skill_club, skill_axe, skill_sword, skill_dist, skill_shield, skill_fish) VALUES (" + int2str(ACCID) + ", '" + Database::escapeString(name) + "','" + Database::escapeString(voc) + "'," + int2str(LVL) + "," + int2str(EXP) + "," + int2str(ML) + "," + int2str(S0) + "," + int2str(S1) + "," + int2str(S2) + "," + int2str(S3) + "," + int2str(S4) + "," + int2str(S5) + "," + int2str(S6) + ")");
						q.free_result();
					}
					else
					{
						q.free_result();
					}
				}
				q.execute("UPDATE pg LEFT JOIN highscores ON highscores.account_id = pg.account_id SET exp_1 = exp_1 + highscores.exp - pg.exp, pg.exp = highscores.exp");
				q.free_result();
				q.execute("COMMIT");
				q.free_result();
			}

			NetworkMessage writeMsg;
			writeMsg.addByte(0x00); // Error code

			writeMsg.WriteToSocket(socket);
		}
		else
		{
			std::cout << "Unknown packet type: " << (int) packetId << std::endl;
			parseDebug(msg);
		}
	}

	if(socket){
		closesocket(socket);
	}

	return 0;
}
