#include "StdAfx.h"
#include "SevenCryptLibrary.h"

namespace SevenZip
{
const TString DefaultCryptLibrary = _T( "Cryptdll.dll" );

SevenCryptLibrary::SevenCryptLibrary(void)
	:m_dll(nullptr),
	m_md5Init(nullptr),
	m_md5Update(nullptr),
	m_md5Final(nullptr)
{
}


SevenCryptLibrary::~SevenCryptLibrary(void)
{
	Free();
}

void SevenCryptLibrary::Load()
{
	Load(DefaultCryptLibrary);
}


void SevenCryptLibrary::Load( const TString& libraryPath )
{
	Free();
	m_dll = ::LoadLibrary( libraryPath.c_str() );
	if ( m_dll == NULL )
	{
		throw SevenZipException( GetWinErrMsg( _T( "LoadLibrary" ), GetLastError() ) );
	}

	m_md5Init = reinterpret_cast< TMD5Init >( ::GetProcAddress( m_dll, "MD5Init" ) );
	if ( m_md5Init == NULL )
	{
		Free();
		throw SevenZipException( _T( "Loaded library is missing required MD5Init function" ) );
	}

	m_md5Update = reinterpret_cast< TMD5Update >( ::GetProcAddress( m_dll, "MD5Update" ) );
	if ( m_md5Update == NULL )
	{
		Free();
		throw SevenZipException( _T( "Loaded library is missing required MD5Update function" ) );
	}

	m_md5Final = reinterpret_cast< TMD5Final >( ::GetProcAddress( m_dll, "MD5Final" ) );
	if ( m_md5Final == NULL )
	{
		Free();
		throw SevenZipException( _T( "Loaded library is missing required MD5Final function" ) );
	}

}


void SevenCryptLibrary::Free()
{
	if ( m_dll != nullptr )
	{
		::FreeLibrary( m_dll );
		m_dll = nullptr;
		m_md5Init = nullptr;
		m_md5Update = nullptr;
		m_md5Final = nullptr;
	}
}


void SevenCryptLibrary::MD5Init(MD5_CTX* ctx) const
{
	if (m_md5Init)
	{
		m_md5Init(ctx);
	}
	else
	{
		throw SevenZipException( DefaultCryptLibrary + _T( " is not loaded" ) );
	}
}


void SevenCryptLibrary::MD5Update(MD5_CTX* ctx, const void* data, unsigned int length) const
{
	if (m_md5Update)
	{
		m_md5Update(ctx, data, length);
	}
	else
	{
		throw SevenZipException( DefaultCryptLibrary + _T( " is not loaded" ) );
	}
}


void SevenCryptLibrary::MD5Final(MD5_CTX* ctx) const
{
	if (m_md5Final)
	{
		m_md5Final(ctx);
	}
	else
	{
		throw SevenZipException( DefaultCryptLibrary + _T( " is not loaded" ) );
	}
}

}