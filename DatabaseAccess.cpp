#include "DatabaseAccess.h"
#include "ItemNotFoundException.h"
#include <io.h>

bool DatabaseAccess::open()
{
	int doesFileExist = _access(FILE_NAME, 0);
	int res = sqlite3_open(FILE_NAME, &_db);
	if (res != SQLITE_OK)
	{
		_db = nullptr;
		return false;
	}
	if (!doesFileExist)
	{
		if (!runQuery("CREATE TABLE USERS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,NAME TEXT NOT NULL);"))
		{
			return false;
		}
		if (!runQuery("CREATE TABLE ALBUMS (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, CREATION_DATA TEXT NOT NULL, USER_ID INTEGER NOT NULL, FOREIGN KEY (USER_ID) REFERENCES USERS(ID));"))
		{
			return false;
		}
		if (!runQuery("CREATE TABLE PICTURES (ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CREATION_DATA TEXT NOT NULL, ALBUM_ID INTEGER NOT NULL, FOREIGN KEY(ALBUM_ID) REFERENCES ALBUMS(ID));"))
		{
			return false;
		}
		if (!runQuery("CREATE TABLE TAGS(ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,PICTURE_ID INTEGER NOT NULL,USER_ID INTEGER NOT NULL,FOREIGN KEY(PICTURE_ID) REFERENCES PICTURES(ID),FOREIGN KEY(USER_ID) REFERENCES USERS(ID));"))
		{
			return false;
		}
	}
	return true;
}

void DatabaseAccess::close()
{
	sqlite3_close(_db);
	_db = nullptr;
}

void DatabaseAccess::clear()
{
	remove(FILE_NAME);
	open();
}

void DatabaseAccess::createUser(User & user)
{
	const static char* query = "INSERT INTO USERS (ID, NAME) VALUES (?, ?)";
	const char* name = user.getName().c_str();
	int id = user.getId();
	if (exec(_db, query, nullptr, nullptr, nullptr, "IT", { &name, &id }) != SQLITE_OK)
	{

		std::cerr << "Couldn't add user" << std::endl;
	}
	/*
	sqlite3_stmt* stmt;
	const char *pzTest;
	int rc = sqlite3_prepare(_db, query, strlen(query), &stmt, &pzTest);
	if (rc == SQLITE_OK) {
		//http://www.askyb.com/cpp/c-sqlite-example-with-parameterized-query/
		sqlite3_bind_int(stmt, 1, user.getId());
		sqlite3_bind_text(stmt, 2, user.getName().c_str(), strlen(user.getName().c_str()), 0);
		sqlite3_step(stmt);//Running the stmt
		sqlite3_finalize(stmt);
	}
	else
		std::cerr << "Couldn't add user" << std::endl;
	*/
}

User DatabaseAccess::getUser(int userId)
{
	const static char* query = "SELECT NAME FROM USERS WHERE ID=?";
	std::string name("");
	static auto callback = [](void* data, int len,const char** value,const char** colm) -> int {
		std::string* pointMe = (std::string*) data;
		*pointMe = std::string(value[0]);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &name, nullptr, "I", { &userId });
	if (!name.empty())
	{
		User usr(userId, name);
		return usr;
	}
	/*
	sqlite3_stmt* stmt;
	const char *pzTest;
	int rc = sqlite3_prepare(_db, query, strlen(query), &stmt, &pzTest);
	if (rc == SQLITE_OK)
	{
		//http://www.askyb.com/cpp/c-sqlite-example-with-parameterized-query/
		//https://stackoverflow.com/questions/31745465/how-to-prepare-sql-statements-and-bind-parameters
		sqlite3_bind_int(stmt, 1, userId);

		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			const unsigned char* unsignedName = sqlite3_column_text(stmt, 1);
			int len = 0;
			for (len = 0; unsignedName[len] != 0; len++);
			char* charName = new char[len + 1];
			for (int i = 0; i < len + 1; charName[i] = unsignedName[i], i++);
			std::string name(charName);
			delete charName;
			User usr(userId , name);
			sqlite3_finalize(stmt);
			return usr;
		}
		sqlite3_finalize(stmt);
	}
	*/
	throw ItemNotFoundException("User", userId);
}

void DatabaseAccess::deleteUser(const User & user)
{
	const static char* query = "DELETE FROM USERS WHERE ID=?";
	const static char* query2 = "DELETE FROM TAGS WHERE USER_ID=?";
	const static char* query3 = "SELECT ID, USER_ID FROM ALBUMS WHERE USER_ID=?";
	int id = user.getId();
	static auto removeUser = [this](const std::string & albumName, int userId)-> void {deleteAlbum(albumName, userId); };//Hacky hack
	static auto callback = [](void* pData, int len,const char** value,const char** colm) -> int {
		removeUser(std::string(value[0]), atoi(value[1]));
		return SQLITE_OK;
	};
	exec(_db, query3, callback, nullptr, nullptr, "I", { &id });
	exec(_db, query2, nullptr, nullptr, nullptr, "I", { &id });
	exec(_db, query, nullptr, nullptr, nullptr, "I", { &id });

}

bool DatabaseAccess::doesUserExists(int userId)
{
	const static char* query = "SELECT NAME FROM USERS WHERE ID=?";
	sqlite3_stmt* stmt;
	const char *pzTest;
	int rc = sqlite3_prepare(_db, query, strlen(query), &stmt, &pzTest);
	if (rc == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, userId);
		bool result = sqlite3_step(stmt) == SQLITE_ROW;
		sqlite3_finalize(stmt);
		return result;
	}
	return false;
}

int DatabaseAccess::countAlbumsOwnedOfUser(const User & user)
{
	const static char* query = "SELECT COUNT(ID) FROM ALBUMS WHERE USER_ID=?";
	int userId = user.getId();
	int result = 0;
	static auto callback = [](void* result, int len,const char** value,const char** name) -> int {
		*((int*)result) = atoi(value[0]);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &result, nullptr, "I", {&userId});
	return result;
}

int DatabaseAccess::countAlbumsTaggedOfUser(const User & user)
{
	const static char* query = "SELECT COUNT(DISTINCT ALBUMS.ID) FROM ALBUMS INNER JOIN PICTURES ON ALBUMS.ID=PICTURES.ALBUM_ID	INNER JOIN TAGS	ON PICTURES.ID=TAGS.PICTURE_ID WHERE TAGS.USER_ID=?";
	int userId = user.getId();
	int result = 0;
	static auto callback = [](void* result, int len, const char** value, const char** name) -> int {
		*((int*)result) = atoi(value[0]);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &result, nullptr, "I", { &userId });
	return result;
}

int DatabaseAccess::countTagsOfUser(const User & user)
{
	//In my table there is IDs for tags even though in the example data set you gave us there isn't an ID colum
	const static char* query = "SELECT COUNT(ID) FROM TAGS WHERE USER_ID=?";
	int userId = user.getId();
	int result = 0;
	static auto callback = [](void* result, int len, const char** value, const char** name) -> int {
		*((int*)result) = atoi(value[0]);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &result, nullptr, "I", { &userId });
	return result;
}

float DatabaseAccess::averageTagsPerAlbumOfUser(const User & user)
{
	const static char* query = "SELECT 1.0*COUNT(PICTURE_ID)/COUNT(DISTINCT ALBUMS.ID) FROM ALBUMS INNER JOIN PICTURES ON ALBUMS.ID=PICTURES.ALBUM_ID INNER JOIN TAGS ON PICTURES.ID=TAGS.PICTURE_ID WHERE TAGS.USER_ID=?";
	float userId = user.getId();
	int result = 0;
	static auto callback = [](void* result, int len, const char** value, const char** name) -> int {
		*((float*)result) = atof(value[0]);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &result, nullptr, "I", { &userId });
	return result;
}

User DatabaseAccess::getTopTaggedUser()
{
	const static char* query = "SELECT ID, NAME FROM USERS WHERE ID=(SELECT USER_ID FROM TAGS GROUP BY USER_ID ORDER BY COUNT(ID) DESC LIMIT 1)";
	User m8(-1, "");
	static auto callback = [](void* user, int len, char** value, char** colm) -> int {
		*((User*)user) = User(atoi(value[0]), std::string(value[1]));
		return SQLITE_OK;
	};
	sqlite3_exec(_db, query, callback, &m8, nullptr);
	if (m8.getId() == -1)
		throw MyException("No tagged users");
	return m8;
}

Picture DatabaseAccess::getTopTaggedPicture()
{
	const static char* query = "SELECT ID, NAME, LOCATION, CREATION_DATE FROM PICTURES WHERE ID=(SELECT PICTURE_ID FROM TAGS GROUP BY PICTURE_ID ORDER BY COUNT(ID) DESC LIMIT 1)";
	Picture m8(-1, "", "", "");
	static auto addData = [this](Picture& pic)-> void {addPictureData(pic); };//Hacky hack
	static auto callback = [](void* pId, int len, char** text, char** colm) -> int {
		Picture pic(atoi(text[0]), std::string(text[1]), std::string(text[2]), std::string(text[3]));
		addData(pic);
		*((Picture*)pId) = pic;
		return SQLITE_OK;
	};
	sqlite3_exec(_db, query, callback, &m8, nullptr);
	if (m8.getId() == -1)
		throw MyException("No tagged pictures");
	return m8;
}

std::list<Picture> DatabaseAccess::getTaggedPicturesOfUser(const User & user)
{
	const static char* query = "SELECT PICTURES.ID, NAME, LOCATION, CREATION_DATE FROM PICTURES INNER JOIN TAGS ON TAGS.PICTURE_ID = PICTURES.ID WHERE TAGS.USER_ID=?";
	std::list<Picture> list;
	int id = user.getId();
	static auto addData = [this](Picture& pic)-> void {addPictureData(pic); };//Hacky hack
	static auto callback = [](void* list, int len, const char** text, const char** colm) -> int {
		Picture pic(atoi(text[0]), std::string(text[1]), std::string(text[2]), std::string(text[3]));
		addData(pic);
		((std::list<Picture>*) list)->push_back(pic);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &list, nullptr, "I", {&id});
	return list;
}

const std::list<Album> DatabaseAccess::getAlbums()
{
	const static char* query = "SELECT USER_ID, NAME, CREATION_DATE FROM ALBUMS";
	std::list<Album> albums;
	static auto addData = [this](Album& alb)-> void {addAlbumData(alb); };//Hacky hack
	static auto callback = [](void* list, int len, char** value, char** colm) -> int {
		Album alb(atoi(value[0]), std::string(value[1]), std::string(value[2]));
		addData(alb);
		((std::list<Album>*) list)->push_back(alb);
		return SQLITE_OK;
	};
	sqlite3_exec(_db, query, callback, &albums, nullptr);
	return albums;
}

const std::list<Album> DatabaseAccess::getAlbumsOfUser(const User & user)
{
	const static char* query = "SELECT USER_ID, NAME, CREATION_DATE FROM ALBUMS WHERE USER_ID=?";
	std::list<Album> albums;
	int userId = user.getId();
	static auto addData = [this](Album& alb)-> void {addAlbumData(alb); };//Hacky hack
	static auto callback = [](void* list, int len, const char** value, const char** colm) -> int {
		Album alb(atoi(value[0]), std::string(value[1]), std::string(value[2]));
		addData(alb);
		((std::list<Album>*) list)->push_back(alb);
		return SQLITE_OK;
	};
	exec(_db, query, callback, &albums, nullptr, "I", { &userId });
	return albums;
}

void DatabaseAccess::createAlbum(const Album & album)
{
	const static char* query = "INSERT INTO ALBUMS (NAME, CREATION_DATE, USER_ID) VALUES (?, ?, ?)";
	const char* name = album.getName().c_str();
	const char* date = album.getCreationDate().c_str();
	int userId = album.getOwnerId();
	exec(_db, query, nullptr, nullptr, nullptr, "TTI", { &name, &date, &userId });
}

void DatabaseAccess::deleteAlbum(const std::string & albumName, int userId)
{
	const static char* query = "DELETE FROM ALBUMS WHERE USER_ID=? AND NAME=?";
	//NAME THAN ID
	const static char* query2 = "SELECT ALBUMS.NAME, PICTURES.NAME FROM ALBUMS INNER JOIN PICTURES ON ALBUMS.ID=PICTURES.ALBUM_ID WHERE ALBUMS.NAME=? AND ALBUMS.USER_ID=?;";
	const char* name = albumName.c_str();
	static auto removePicture = [this](const std::string& albumName, const std::string& pictureName)-> void {removePictureFromAlbumByName(albumName, pictureName); };//Hacky hack
	static auto callback = [](void* pData, int len, const char** data, const char** name) -> int {
		removePicture(std::string(data[0]), std::string(data[1]));
		return SQLITE_OK;
	};
	exec(_db, query2, callback, nullptr, nullptr, "TI", { &name, &userId });
	exec(_db, query, nullptr, nullptr, nullptr, "IT", { &userId, &name });
}

bool DatabaseAccess::doesAlbumExists(const std::string & albumName, int userId)
{
	const static char* query = "SELECT NAME FROM ALBUMS WHERE USER_ID=? AND NAME=?";
	sqlite3_stmt* stmt;
	const char *pzTest;
	int rc = sqlite3_prepare(_db, query, strlen(query), &stmt, &pzTest);
	if (rc == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, userId);
		sqlite3_bind_text(stmt, 2, albumName.c_str(), strlen(albumName.c_str()), nullptr);
		bool result = sqlite3_step(stmt) == SQLITE_ROW;
		sqlite3_finalize(stmt);
		return result;
	}
	return false;
}

Album DatabaseAccess::openAlbum(const std::string & albumName)
{
	const static char* query = "SELECT USER_ID, NAME, CREATION_DATE FROM ALBUMS WHERE NAME=?";
	const char* name = albumName.c_str();
	Album album;
	Album* ptr = &album;
	static auto callback = [](void* ptrPtr, int len, const char** value, const char** colm) -> int {
		Album* ptr = *((Album**)ptrPtr);
		*ptr = Album(atoi(value[0]), std::string(value[1]), std::string(value[2]));
		ptr = nullptr;
		return SQLITE_OK;
	};
	exec(_db, query, callback, &ptr, nullptr, "T", { &name });
	if(ptr != nullptr)
		throw MyException("No album with name " + albumName + " exists");
	addAlbumData(album);
	return album;
}

void DatabaseAccess::closeAlbum(Album & pAlbum)
{
	// basically here we would like to delete the allocated memory we got from openAlbum
}

void DatabaseAccess::printAlbums()
{
	//Not the most efficent thing
	std::list<Album> m_albums = getAlbums();
	//Kind of a dumb thing to have the function like I would probably make a parent one at IDataAccess that you can send a list of albums to so it will print it
	if (m_albums.empty()) {
		throw MyException("There are no existing albums.");
	}
	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const Album& album : m_albums) {
		std::cout << std::setw(5) << "* " << album;
	}
}

void DatabaseAccess::addPictureToAlbumByName(const std::string & albumName, const Picture & picture)
{
	static const char* query = "INSERT INTO PICTURES (NAME, LOCATION, CREATION_DATE, ALBUM_ID) VALUES (?, ?, ?, (SELECT ID FROM ALBUMS WHERE NAME=?));";
	const char* albumN = albumName.c_str();
	const char* name = picture.getName().c_str();
	const char* location = picture.getPath().c_str();
	const char* creation_date = picture.getCreationDate().c_str();
	exec(_db, query, nullptr, nullptr, nullptr, "TTTT", { &name, &location, &creation_date, &albumN });
}

void DatabaseAccess::removePictureFromAlbumByName(const std::string & albumName, const std::string & pictureName)
{
	static const char* query = "DELETE FROM PICTURES WHERE NAME=? and ALBUM_ID=(SELECT ID FROM ALBUMS WHERE NAME=?)";
	static const char* query2 = "DELETE FROM TAGS WHERE PICTURE_ID=(SELECT ID FROM PICTURES WHERE ALBUM_ID=(SELECT ID FROM ALBUMS WHERE NAME=?) AND NAME=?)";
	const char* albumN = albumName.c_str();
	const char* pictureN = pictureName.c_str();
	exec(_db, query2, nullptr, nullptr, nullptr, "TT", { &albumN, &pictureN });
	exec(_db, query, nullptr, nullptr, nullptr, "TT", { &pictureN, &albumN });
}

void DatabaseAccess::tagUserInPicture(const std::string & albumName, const std::string & pictureName, int userId)
{
	//ALBUM PICTURE userId
	static const char* query = "INSERT INTO TAGS (PICTURE_ID, USER_ID) VALUES ((SELECT ID FROM PICTURES WHERE ALBUM_ID=(SELECT ID FROM ALBUMS WHERE NAME=?) AND NAME=?), ?)";
	const char* albumN = albumName.c_str();
	const char* pictureN = pictureName.c_str();
	exec(_db, query, nullptr, nullptr, nullptr, "TTI", { &albumN, &pictureN, &userId });
}

void DatabaseAccess::untagUserInPicture(const std::string & albumName, const std::string & pictureName, int userId)
{
	//ALBUM PICTURE userId
	static const char* query = "DELETE FROM TAGS WHERE PICTURE_ID=(SELECT ID FROM PICTURES WHERE ALBUM_ID=(SELECT ID FROM ALBUMS WHERE NAME=?) AND NAME=?) AND USER_ID=?";
	const char* albumN = albumName.c_str();
	const char* pictureN = pictureName.c_str();
	exec(_db, query, nullptr, nullptr, nullptr, "TTI", { &albumN, &pictureN, &userId });
}

void DatabaseAccess::printUsers()
{
	std::vector<User> m_users = getAllUsers();
	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const auto& user : m_users) {
		std::cout << user << std::endl;
	}
}

void DatabaseAccess::addAlbumData(Album & alb)
{
	//Bad structure why album doesn't have an ID;
	const static char* query = "SELECT ID, NAME, LOCATION, CREATION_DATE FROM PICTURES WHERE ALBUM_ID=(SELECT ID FROM ALBUMS WHERE USER_ID=? AND NAME=?);";
	int userId = alb.getOwnerId();
	std::string name = alb.getName();
	static auto addData = [this](Picture& pic)-> void {addPictureData(pic); };//Hacky hack
	auto callback = [](void* album , int len, const char** text, const char** name) -> int {
		Picture pic(atoi(text[0]), std::string(text[1]), std::string(text[2]), std::string(text[3]));
		addData(pic);
		((Album*)album)->addPicture(pic);
		return 0;
	};
	exec(_db, query, callback, &alb, nullptr, "IS", {&userId, &name});
}

void DatabaseAccess::addPictureData(Picture & pic)
{
	const static char* query = "SELECT USER_ID FROM TAGS WHERE PICTURE_ID=?";
	int pic_id = pic.getId();
	static auto callback = [](void* picure, int len, const char** text, const char** name) -> int {
		((Picture*)picure)->tagUser(atoi(text[0]));
		return 0;
	};
	exec(_db, query, callback, &pic, nullptr, "I", { &pic_id });
}

int DatabaseAccess::exec(sqlite3 * sql, const char * query, int(*callback)(void *, int,const char **,const char **), void * val, char ** errmsg, const char * types, std::vector<void*> args)
{
	const static auto writeNotNull = [](char** errMsg, const char* msg) {
		if (errMsg != nullptr)
		{
			int len = 0;
			for (len = 0; msg[len] != 0; len++);
			*errMsg = new char[len];
			for (len = 0; (*errMsg[len] = msg[len]) != 0; len++);
		}
	};
	sqlite3_stmt* stmt;
	const char *pzTest;
	int rc = sqlite3_prepare_v2(_db, query, strlen(query), &stmt, &pzTest);
	if (rc == SQLITE_OK)
	{
		for (int pos = 0; types[pos] != 0; pos++)
		{
			switch (types[pos])
			{
			case 'I':
			case 'i':
				rc = sqlite3_bind_int(stmt, pos + 1, *((int*)args[pos]));
				break;
			case 'D':
			case 'd':
				rc = sqlite3_bind_double(stmt, pos + 1, *((double*)args[pos]));
				break;
			case 'T':
			case 't':
			{
				char* text = *((char**)args[pos]);
				rc = sqlite3_bind_text(stmt, pos + 1, text, strlen(text), nullptr);
			}
				break;
			case 'S':
			case 's':
			{
				const char* text = ((std::string*)args[pos])->c_str();
				rc = sqlite3_bind_text(stmt, pos + 1, text, strlen(text), nullptr);
			}
				break;
			}
			if (rc != SQLITE_OK)
			{
				writeNotNull(errmsg, "Error while binding the values");
				sqlite3_finalize(stmt);
				return rc;
			}
		}
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			if (callback == nullptr)
				continue;
			int colNum = sqlite3_column_count(stmt);
			const char** text = new const char*[colNum];
			const char** names = new const char*[colNum];
			for (int i = 0; i < colNum; i++)
			{
				text[i] = (const char*) sqlite3_column_text(stmt, i);
				names[i] = (const char*)sqlite3_column_name(stmt, i);
			}
			rc = callback(val, colNum, text, names);
			delete text;
			delete names;
			if (rc != SQLITE_OK)
			{
				writeNotNull(errmsg, "Error while executing callback function");
				sqlite3_finalize(stmt);
				return rc;
			}
		}
		sqlite3_finalize(stmt);
	}
	return rc;
}

std::vector<User> DatabaseAccess::getAllUsers()
{
	std::vector<User> users;
	//Static means I dont have to recreate the function every time;
	static auto callback = [](void* data, int len, char** value, char** colm) -> int {
		int id = atoi(value[0]);
		std::string name(value[1]);
		std::vector<User>* users = (std::vector<User>*)data;
		users->push_back(User(id, name));
		return SQLITE_OK; 
	};
	char* charm8 = "SELECT * FROM USERS;";
	sqlite3_exec(_db, charm8, callback, &users, nullptr);
	return users;
}

bool DatabaseAccess::runQuery(char * query)
{
	return sqlite3_exec(_db, query, nullptr, nullptr, nullptr) == SQLITE_OK;
}
