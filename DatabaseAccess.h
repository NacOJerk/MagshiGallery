#pragma once
#include "sqlite3.h"
#include "IDataAccess.h"
#include <vector>

#define FILE_NAME "test.db"

class DatabaseAccess :
	public IDataAccess
{
public:
	DatabaseAccess() = default;
	virtual ~DatabaseAccess() = default;

	//Album related
	const std::list<Album> getAlbums() override;
	const std::list<Album> getAlbumsOfUser(const User& user) override;
	void createAlbum(const Album& album) override;
	void deleteAlbum(const std::string& albumName, int userId) override;
	bool doesAlbumExists(const std::string& albumName, int userId) override;
	Album openAlbum(const std::string& albumName) override;
	void closeAlbum(Album& pAlbum) override;
	void printAlbums() override;

	//PictureRelated
	void addPictureToAlbumByName(const std::string& albumName, const Picture& picture) override;//Doesn't support pictures with already tagged users
	void removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) override;
	void tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;
	void untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId) override;

	//User related
	void printUsers() override;
	void createUser(User& user) override;
	User getUser(int userId) override;
	void deleteUser(const User& user) override;
	bool doesUserExists(int userId) override;
	
	// user statistics
	int countAlbumsOwnedOfUser(const User& user) override;
	int countAlbumsTaggedOfUser(const User& user) override;
	int countTagsOfUser(const User& user) override;
	float averageTagsPerAlbumOfUser(const User& user) override;

	// queries
	User getTopTaggedUser() override;
	Picture getTopTaggedPicture() override;
	std::list<Picture> getTaggedPicturesOfUser(const User& user) override;

	bool open() override;
	void close() override;
	void clear() override;
private:
	void addAlbumData(Album& alb);
	void addPictureData(Picture& pic);

	sqlite3* _db;
	//Same as sqlite3_exec just with bindings and a hella safer will return SQLITE_OK if worked and if not will put error in error msg (if it is not null) the err msg needs to be deallocated after 
	int exec(sqlite3* sql, const char* query, int(*callback)(void*, int, const char **, const char **), void* val, char** errmsg, const char* types, std::vector<void*> args);
	std::vector<User> getAllUsers();
	bool runQuery(char* query);
};

