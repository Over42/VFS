#include "tests.h"

#include <thread>


void TestTask::Test_CreatePaks(VFS* vfs)
{
	const char* pakPath1 = "Test_Files/Pak1.pak";
	const char* filePaths1[] = { "Test_Files/File1.txt", "Test_Files/File2.txt", "Test_Files/File3.txt" };
	vfs->PackFiles(pakPath1, filePaths1, 3, 1);

	const char* pakPath2 = "Test_Files/Pak2.pak";
	const char* filePaths2[] = { "Test_Files/File4.txt", "Test_Files/File5.txt" };
	vfs->PackFiles(pakPath2, filePaths2, 2, 1);
}

void TestTask::Test_OpenFile(VFS* vfs, const char* name)
{
	File* f = vfs->Open(name);
	if (f)
	{
		printf("Open %s -> Size: %d\n", name, f->Size);
		vfs->Close(f);
	}
}

void TestTask::Test_CreateFile(VFS* vfs, const char* name)
{
	File* f = vfs->Create(name);
	if (f)
	{
		printf("Create %s -> Offset: %d\n", name, f->Offset);
		char buff[] = "File4 constents: New file created";
		vfs->Write(f, buff, sizeof(buff));
		vfs->Close(f);
	}
}

void TestTask::Test_ReadFile(VFS* vfs, const char* name, char* buff, size_t len)
{
	File* f = vfs->Open(name);
	if (f)
	{
		size_t bytesReaded = vfs->Read(f, buff, len);
		printf("Read %s -> Bytes readed: %d; Contents: %s\n", name, bytesReaded, buff);
		vfs->Close(f);
	}
}

void TestTask::Test_WriteFile(VFS* vfs, const char* name, char* buff, size_t len)
{
	File* f = vfs->Create(name);
	if (f)
	{
		size_t bytesWritten = vfs->Write(f, buff, len);
		printf("Write %s -> Bytes written: %d\n", name, bytesWritten);
		vfs->Close(f);
	}
}

void TestTask::Test_AllMethods(VFS* vfs)
{
	printf("\n===== Basic test =====\n");

	Test_OpenFile(vfs, "Test_Files/File1.txt");
	Test_OpenFile(vfs, "Test_Files/File5.txt");
	Test_CreateFile(vfs, "Test_Files/New/Path/File4.txt");
	char readBuff1[100] = {};
	Test_ReadFile(vfs, "Test_Files/File3.txt", readBuff1, sizeof(readBuff1));
	char readBuff2[100] = {};
	Test_ReadFile(vfs, "Test_Files/File4.txt", readBuff2, sizeof(readBuff2));
	char writeBuff[] = "File2 CONTENTS: World!";
	Test_WriteFile(vfs, "Test_Files/File2.txt", writeBuff, sizeof(writeBuff));
}

void TestTask::Test_MultithreadedReadWrite(VFS* vfs)
{
	printf("\n===== Multithreaded test =====\n");

	printf("=== Read same file ===\n");
	char readBuff1[100] = {};
	char readBuff2[100] = {};
	std::thread reader1(Test_ReadFile, vfs, "Test_Files/File3.txt", readBuff1, sizeof(readBuff1));
	std::thread reader2(Test_ReadFile, vfs, "Test_Files/File3.txt", readBuff2, sizeof(readBuff2));

	printf("=== Write same file ===\n");
	char writeBuff1[] = "File2 CONTENTS: thrd1!";
	std::thread writer1(Test_WriteFile, vfs, "Test_Files/File2.txt", writeBuff1, sizeof(writeBuff1));
	char writeBuff2[] = "File2 CONTENTS: thrd2!";
	std::thread writer2(Test_WriteFile, vfs, "Test_Files/File2.txt", writeBuff2, sizeof(writeBuff2));
	
	reader1.join();
	reader2.join();
	writer1.join();
	writer2.join();

	printf("=== Read and Write same file ===\n");
	char readBuff3[100] = {};
	char writeBuff3[] = "File1 contents: Byeee";
	std::thread reader3(Test_ReadFile, vfs, "Test_Files/File1.txt", readBuff3, sizeof(readBuff3));
	std::thread writer3(Test_WriteFile, vfs, "Test_Files/File1.txt", writeBuff3, sizeof(writeBuff3));
	reader3.join();
	writer3.join();
}
