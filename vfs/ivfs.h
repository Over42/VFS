#pragma once

namespace TestTask
{
struct File;

struct IVFS
{
	virtual ~IVFS() = default;

	// ������� ���� � readonly ������.
	// ���� ��� ������ ����� ��� �� �� ������ �� writeonly ������ - ������� nullptr
	virtual File* Open(const char* name) = 0;

	// ������� ��� ������� ���� � writeonly ������. ���� �����, �� ������� ��� ������ �������������, ���������� � ����.
	// ������� nullptr, ���� ���� ���� ��� ������ � readonly ������.
	virtual File* Create(const char* name) = 0;

	// ��������� ������ �� �����.
	// ������������ �������� - ������� ������� ���� ������� ���������
	virtual size_t Read(File* f, char* buff, size_t len) = 0;

	// �������� ������ � ����.
	// ������������ �������� - ������� ������� ���� ������� ��������
	virtual size_t Write(File* f, char* buff, size_t len) = 0;

	// ������� ����
	virtual void Close(File* f) = 0;	
};
}
