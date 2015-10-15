#include "stdafx.h"
#include <7zip/ICoder.h>
#include "SevenZipLibrary.h"


namespace SevenZip
{

const TString DefaultSevenZipLibrary = _T( "7z.dll" );

SevenZipLibrary::SevenZipLibrary()
	: m_dll( NULL )
	, m_func( NULL )
{
}

SevenZipLibrary::~SevenZipLibrary()
{
	Free();
}

void SevenZipLibrary::Load()
{
	Load( DefaultSevenZipLibrary );
}

void SevenZipLibrary::Load( const TString& libraryPath )
{
	Free();
	m_dll = LoadLibrary( libraryPath.c_str() );
	if ( m_dll == NULL )
	{
		throw SevenZipException( GetWinErrMsg( _T( "LoadLibrary" ), GetLastError() ) );
	}

	m_func = reinterpret_cast< TCreateObject >( GetProcAddress( m_dll, "CreateObject" ) );
	if ( m_func == NULL )
	{
		Free();
		throw SevenZipException( _T( "Loaded library is missing required CreateObject function" ) );
	}

	m_hashers = reinterpret_cast< TGetHashers >( GetProcAddress( m_dll, "GetHashers" ) );
	if ( m_hashers == NULL )
	{
		Free();
		throw SevenZipException( _T( "Loaded library is missing required GetHashers function" ) );
	}
}

void SevenZipLibrary::Free()
{
	if ( m_dll != NULL )
	{
		FreeLibrary( m_dll );
		m_dll = NULL;
		m_func = NULL;
	}
}

void SevenZipLibrary::CreateObject( const GUID& clsID, const GUID& interfaceID, void** outObject ) const
{
	if ( m_func == NULL )
	{
		throw SevenZipException( DefaultSevenZipLibrary + _T( " is not loaded" ) );
	}

	HRESULT hr = m_func( &clsID, &interfaceID, outObject );
	if ( FAILED( hr ) )
	{
		throw SevenZipException( GetCOMErrMsg( _T( "CreateObject" ), hr ) );
	}
}

void SevenZipLibrary::GetHashers(IHashers **hashers) const
{
	if ( m_hashers == NULL )
	{
		throw SevenZipException( DefaultSevenZipLibrary + _T( " is not loaded" ) );
	}
	
	auto hr = m_hashers(hashers);
	if ( FAILED( hr ) )
	{
		throw SevenZipException( GetCOMErrMsg( _T( "GetHashers" ), hr ) );
	}
}

}
