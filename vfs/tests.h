#pragma once

#include "vfs.h"

namespace TestTask
{
	void Test_CreatePaks(VFS* vfs);
	void Test_OpenFile(VFS* vfs, const char* name);
	void Test_CreateFile(VFS* vfs, const char* name);
	void Test_ReadFile(VFS* vfs, const char* name, char* buff, size_t len);
	void Test_WriteFile(VFS* vfs, const char* name, char* buff, size_t len);

	void Test_AllMethods(VFS* vfs);
	void Test_MultithreadedReadWrite(VFS* vfs);
}
