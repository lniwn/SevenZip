#pragma once
#include "SevenZipException.h"

namespace SevenZip
{
typedef struct 
{
	ULONG i[2]; 
	ULONG buf[4]; 
	unsigned char in[64]; 
	unsigned char digest[16]; 
}MD5_CTX;

class SevenCryptLibrary
{
private:
	typedef void (WINAPI *TMD5Init)(MD5_CTX*);
	typedef void (WINAPI *TMD5Update)(MD5_CTX*, const void*, unsigned int);
	typedef void (WINAPI *TMD5Final)(MD5_CTX*);

	SevenCryptLibrary(const SevenCryptLibrary&);
	SevenCryptLibrary& operator=(const SevenCryptLibrary&);

public:
	SevenCryptLibrary(void);
	~SevenCryptLibrary(void);

	void Load();
	void Load( const TString& libraryPath );
	void Free();

	void MD5Init(MD5_CTX*) const;
	void MD5Update(MD5_CTX*, const void*, unsigned int) const;
	void MD5Final(MD5_CTX*) const;

private:
	HMODULE m_dll;
	TMD5Init m_md5Init;
	TMD5Update m_md5Update;
	TMD5Final m_md5Final;
};

}