#pragma once

#include "SevenZipException.h"

struct IHashers;
namespace SevenZip
{
	class SevenZipLibrary
	{
	private:
		SevenZipLibrary(const SevenZipLibrary&);
		SevenZipLibrary& operator=(const SevenZipLibrary&);

	public:
		SevenZipLibrary();
		~SevenZipLibrary();

		void Load();
		void Load( const TString& libraryPath );
		void Free();

		void CreateObject( const GUID& clsID, const GUID& interfaceID, void** outObject ) const;
		void GetHashers(IHashers **) const;

	private:

		typedef UINT32 (WINAPI * TCreateObject)( const GUID* clsID, const GUID* interfaceID, void** outObject );
		typedef HRESULT (WINAPI *TGetHashers)(IHashers **);

		HMODULE			m_dll;
		TCreateObject	m_func;
		TGetHashers		m_hashers;
	};
}
