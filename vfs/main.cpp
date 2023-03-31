#include "vfs.h"
#include "tests.h"


using namespace TestTask;

int main(int argc, char *argv[])
{
	VFS vfs;

	Test_CreatePaks(&vfs);

	std::vector<char> pakBuff1;
	FILE* fp1 = vfs.OpenAndReadPak("Test_Files/Pak1.pak", &pakBuff1);
	std::vector<char> pakBuff2;
	FILE* fp2 = vfs.OpenAndReadPak("Test_Files/Pak2.pak", &pakBuff2);

	Test_AllMethods(&vfs);
	Test_MultithreadedReadWrite(&vfs);

	vfs.SaveAndClosePak(fp1, "Test_Files/Pak1.pak", &pakBuff1);
	vfs.SaveAndClosePak(fp2, "Test_Files/Pak2.pak", &pakBuff2);

	return 0;
}
