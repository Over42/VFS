#pragma once

#include <stdint.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <mutex>

#include "ivfs.h"


namespace TestTask
{
constexpr int FILE_VERSION = 1;
constexpr int MAX_FILES_IN_PACK = 10;
constexpr int PATH_SIZE = 255;

struct PakFile
{
	char Format[4] = { "PAK" };
	char Version = 0;
	uint32_t ContentVersion = 0;
	char Path[PATH_SIZE];
	uint32_t NumEntries = 0;
};

struct PakMetadata
{
	char Path[PATH_SIZE];
	FILE* pakFileHandle;
	std::vector<char>* pakBuffer;
	std::mutex rwMutex;

	PakMetadata& operator=(const PakMetadata& o)
	{
		std::strncpy(Path, o.Path, sizeof(Path));
		pakFileHandle = o.pakFileHandle;
		pakBuffer = o.pakBuffer;
		return *this;
	}
};

struct File
{
	char Path[PATH_SIZE];
	bool Compressed = false;
	uint32_t Size = 0;
	uint32_t Offset = 0;
};

struct FileMetadata
{
	char PakPath[PATH_SIZE];
	bool ReadOnly = false;
	bool WriteOnly = false;
	uint32_t NumFiles = 0;
	uint32_t Offset = 0;
	std::mutex rwMutex;

	FileMetadata& operator=(const FileMetadata& o)
	{
		std::strncpy(PakPath, o.PakPath, sizeof(PakPath));
		ReadOnly = o.ReadOnly;
		WriteOnly = o.WriteOnly;
		NumFiles = o.NumFiles;
		Offset = o.Offset;
		return *this;
	}
};

class VFS : IVFS
{
public:
	File* Open(const char* name) override;
	File* Create(const char* name) override;
	size_t Read(File* f, char* buff, size_t len) override;
	size_t Write(File* f, char* buff, size_t len) override;
	void Close(File* f) override;

	void PackFiles(const char* pakPath, const char* filePaths[], size_t fileCount, int contentVersion);
	FILE* OpenAndReadPak(const char* name, std::vector<char>* buff);
	void SaveAndClosePak(FILE* file, const char* name, std::vector<char>* buff);

private:
	std::unordered_map<std::string, FileMetadata> filesMetadata;
	std::unordered_map<std::string, PakMetadata> paksMetadata;
	char unfilledPak[255];  // New files added here

	File* ReadHeader(const char* name);
};

}
