#pragma once

enum EncType
{
	NO_ENC,
	DEFXOR
};

class Configuration {
public:
	static bool Configure();
	static bool InitializeUser(const char* user, const char* password, EncType enc);
	static bool AddDBToUser(const char* dataBaseName, const char* user);
};

class DataBase
{
public:
	DataBase();
	bool OpenDataBase(const char* baseName);
	bool CreateDataBase(const char* user, const char* baseName);
	~DataBase();

private:
	const char* baseName;
};
