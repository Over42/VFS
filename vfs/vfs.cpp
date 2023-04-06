#include "vfs.h"

#include <stdio.h>
#include <vector>
#include <iterator>
// For testing
#include <thread>
#include <chrono>


namespace TestTask
{
File* VFS::Open(const char* name)
{
	auto fileMetadata = filesMetadata.find(name);
	bool fileExists = fileMetadata != filesMetadata.end();
	if (fileExists)
	{
		{
			std::lock_guard<std::mutex> lock(fileMetadata->second.rwMutex);
			if (fileMetadata->second.WriteOnly)
			{
				fprintf(stderr, "ERROR: File already opened in WriteOnly\n");
				return nullptr;
			}
			fileMetadata->second.ReadOnly = true;
		}

		File* file = ReadHeader(name);
		return file;
	}
	else
	{
		fprintf(stderr, "ERROR: File not found\n");
		return nullptr;
	}
}

File* VFS::Create(const char* name)
{
	auto fileMetadata = filesMetadata.find(name);
	bool fileExists = fileMetadata != filesMetadata.end();
	if (fileExists)
	{
		std::lock_guard<std::mutex> lock(fileMetadata->second.rwMutex);
		if (fileMetadata->second.ReadOnly)
		{
			fprintf(stderr, "ERROR: File already opened in ReadOnly\n");
			return nullptr;
		}
		fileMetadata->second.WriteOnly = true;
		File* file = ReadHeader(name);
		return file;
	}
	else
	{
		if (unfilledPak[0] != '\0')
		{
			// Add info to VFS
			FileMetadata metadata = {};
			std::strncpy(metadata.PakPath, unfilledPak, sizeof(metadata.PakPath));
			metadata.NumFiles++;
			filesMetadata[name] = metadata;

			std::lock_guard<std::mutex> lock(filesMetadata[name].rwMutex);
			filesMetadata[name].WriteOnly = true;

			std::lock_guard<std::mutex> pakLock(paksMetadata[unfilledPak].rwMutex);
			// Create file header
			File* file = new File();
			std::strncpy(file->Path, name, sizeof(file->Path));
			file->Size = 0;
			uint32_t offset = paksMetadata[unfilledPak].pakBuffer->size();
			file->Offset = offset;
			filesMetadata[name].Offset = offset;
			
			// Add to Pak buffer
			std::vector<char>* pakBuffer = paksMetadata[unfilledPak].pakBuffer;
			pakBuffer->reserve(sizeof(File));
			char* pfile = (char*)file;
			pakBuffer->insert(pakBuffer->end(), pfile, pfile + sizeof(File));

			return file;
		}
		else
		{
			fprintf(stderr, "ERROR: Can't create file - all Paks filled, create new one\n");
			return nullptr;
		}
	}
}

File* VFS::ReadHeader(const char* name)
{
	auto fileMetadata = filesMetadata.find(name);
	char* pakPath = fileMetadata->second.PakPath;
	uint32_t offset = fileMetadata->second.Offset;

	File* file = new File();
	char* readBuffer = paksMetadata.find(pakPath)->second.pakBuffer->data();
	char* src = &(readBuffer[offset]);
	memcpy(file, src, sizeof(File));

	return file;
}

size_t VFS::Read(File* f, char* buff, size_t len)
{
	// Threads test
	std::thread::id tid = std::this_thread::get_id();
	printf("thread %d reading %s\n", tid, f->Path);
	std::this_thread::sleep_for(std::chrono::seconds(5));

	if (len <= f->Size)
	{
		fprintf(stderr, "ERROR: %s can't be readed - buffer size too small\n", f->Path);
		return 0;
	}

	if (filesMetadata.find(f->Path)->second.WriteOnly)
	{
		fprintf(stderr, "ERROR: can't read - file opened in WriteOnly\n");
		return 0;
	}

	char* pakPath = filesMetadata.find(f->Path)->second.PakPath;
	uint32_t offset = f->Offset + sizeof(File);
	char* readBuffer = paksMetadata.find(pakPath)->second.pakBuffer->data();
	char* src = &(readBuffer[offset]);
	memcpy(buff, src, f->Size);
	return f->Size;
}

size_t VFS::Write(File* f, char* buff, size_t len)
{
	std::lock_guard<std::mutex> lock(filesMetadata.find(f->Path)->second.rwMutex);

	// Threads test
	std::thread::id tid = std::this_thread::get_id();
	printf("thread %d writing %s\n", tid, f->Path);
	std::this_thread::sleep_for(std::chrono::seconds(5));
	
	size_t writeLen = len - 1;  // without '\0'

	bool newFile = f->Size == 0;
	if (newFile)
	{
		std::lock_guard<std::mutex> pakLock(paksMetadata[unfilledPak].rwMutex);
		f->Size = writeLen;
		std::vector<char>* pakBuffer = paksMetadata[unfilledPak].pakBuffer;
		pakBuffer->resize(pakBuffer->size() + f->Size);
	}

	if (writeLen != f->Size)
	{
		fprintf(stderr, "ERROR: %s can't be written - buffer size doesn't match\n", f->Path);
		return 0;
	}

	if (filesMetadata.find(f->Path)->second.ReadOnly)
	{
		fprintf(stderr, "ERROR: can't write - file opened in ReadOnly\n");
		return 0;
	}

	char* pakPath = filesMetadata.find(f->Path)->second.PakPath;
	uint32_t offset = f->Offset + sizeof(File);
	auto pakMetadata = paksMetadata.find(pakPath);
	char* writeBuffer = pakMetadata->second.pakBuffer->data();
	char* dst = &(writeBuffer[offset]);
	memcpy(dst, buff, f->Size);
	return f->Size;
}

void VFS::Close(File* f)
{
	if (f != nullptr)
	{
		filesMetadata.find(f->Path)->second.ReadOnly = false;
		filesMetadata.find(f->Path)->second.WriteOnly = false;
		delete f;
	}
}

void VFS::PackFiles(const char* pakPath, const char* filePaths[], size_t fileCount, int contentVersion)
{
	std::vector<char> buffer;

	PakFile pakHeader = {};
	std::strncpy(pakHeader.Path, pakPath, sizeof(pakHeader.Path));
	pakHeader.NumEntries = 0;
	pakHeader.Version = (char)FILE_VERSION;
	pakHeader.ContentVersion = contentVersion;
	buffer.insert(buffer.end(), (char*)&pakHeader, (char*)&pakHeader + sizeof(pakHeader));

	for (uint32_t i = 0; i < fileCount; i++)
	{
		FILE* fp = fopen(filePaths[i], "rb");
		if (fp)
		{
			// Create file header
			File fileHeader = {};
			fseek(fp, 0, SEEK_END);
			fileHeader.Size = (uint32_t)ftell(fp);
			fseek(fp, 0, SEEK_SET);
			auto offset = (uint32_t)buffer.size();
			fileHeader.Offset = offset;
			std::strncpy(fileHeader.Path, filePaths[i], sizeof(fileHeader.Path));
			buffer.insert(buffer.end(), (char*)&fileHeader, (char*)&fileHeader + sizeof(fileHeader));

			// Read file data in memory
			std::vector<char> fileData;
			fileData.resize(fileHeader.Size);
			fread(fileData.data(), sizeof(char), fileHeader.Size, fp);
			buffer.insert(buffer.end(), fileData.begin(), fileData.end());

			// Add info to VFS
			FileMetadata fileMetadata = {};
			std::strncpy(fileMetadata.PakPath, pakPath, sizeof(fileMetadata.PakPath));
			fileMetadata.NumFiles++;
			fileMetadata.Offset = offset;
			filesMetadata[filePaths[i]] = fileMetadata;

			pakHeader.NumEntries++;

			fclose(fp);
		}
		else
		{
			fprintf(stderr, "ERROR: File doesn't exist\n");
		}
	}

	FILE* pakFile = fopen(pakPath, "wb");
	fwrite(buffer.data(), sizeof(char), buffer.size(), pakFile);
	fclose(pakFile);
	printf("Pak created successfully\n");

	if (pakHeader.NumEntries < MAX_FILES_IN_PACK)
	{
		strncpy(unfilledPak, pakHeader.Path, sizeof(unfilledPak));
	}
	else
	{
		unfilledPak[0] = '\0';
	}
}

FILE* VFS::OpenAndReadPak(const char* name, std::vector<char>* buff)
{
	FILE* file = fopen(name, "rb");
	if (file)
	{
		setvbuf(file, NULL, _IONBF, 0);
		fseek(file, 0, SEEK_END);
		auto buffSize = ftell(file);
		buff->resize(buffSize);
		fseek(file, 0, SEEK_SET);
		fread(buff->data(), sizeof(char), buffSize, file);

		PakMetadata pakMetadata = {};
		pakMetadata.pakFileHandle = file;
		pakMetadata.pakBuffer = buff;
		paksMetadata[name] = pakMetadata;
		return file;
	}
	else
	{
		fprintf(stderr, "ERROR: Pak does not exist\n");
		return nullptr;
	}
}

void VFS::SaveAndClosePak(FILE* file, const char* name, std::vector<char>* buff)
{
	fclose(file);
	FILE* rewrittenFile = fopen(name, "wb");
	fwrite(buff->data(), sizeof(char), buff->size(), rewrittenFile);
	fclose(rewrittenFile);
	buff->clear();
}
}
