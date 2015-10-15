#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "../../SevenZip++/7zpp.h"
#include <fstream>

int PrintUsage()
{
	_tprintf(_T("Simple.exe [cx] ...\n"));
	_tprintf(_T("  c <archiveName> <targetDirectory>      -- Creates an archive.\n"));
	_tprintf(_T("  x <archiveName> <destinationDirectory> -- Extracts an archive.\n"));
	_tprintf(_T("  h <destinationDirectory> <pattern> -- Hashes files in directory.\n\n"));
	return 0;
}


int CreateArchive(int argc, TCHAR** argv)
{
	if (argc < 4)
	{
		return PrintUsage();
	}

	const TCHAR* archiveName = argv[2];
	const TCHAR* targetDir = argv[3];
	
	// Note I'm lazily assuming the target is a directory rather than a file.

	SevenZip::SevenZipLibrary lib;
#ifdef _DEBUG
	lib.Load(_T("7zd.dll"));
#else
	lib.Load(_T("7z.dll"));
#endif // _DEBUG
	
	SevenZip::SevenZipCompressor compressor(lib, archiveName);
	compressor.CompressDirectory(targetDir);
	
	return 0;
}

int HashFiles(int argc, TCHAR** argv)
{
	if (argc < 4)
	{
		return PrintUsage();
	}
	const TCHAR* targetDir = argv[2];
	const TCHAR* pattern = argv[3];

	SevenZip::SevenZipLibrary lib;
#ifdef _DEBUG
	lib.Load(_T("7zd.dll"));
#else
	lib.Load(_T("7z.dll"));
#endif // _DEBUG

	SevenZip::SevenCryptLibrary crypt_lib;
	crypt_lib.Load();
	SevenZip::SevenZipHasher hasherlib(lib, crypt_lib);

	auto hashes = hasherlib.FilesMd5(targetDir, pattern);
	for (auto it = hashes.begin(); it != hashes.end(); ++it)
	{
		_tprintf(_T("%s --> %S\n"), std::get<1>(*it).c_str(), std::get<0>(*it).c_str());
	}
	_tprintf(_T("\n"));

	return 0;
}

int ExtractArchive(int argc, TCHAR** argv)
{
	if (argc < 4)
	{
		return PrintUsage();
	}

	const TCHAR* archiveName = argv[2];
	const TCHAR* destination = argv[3];

	SevenZip::SevenZipLibrary lib;
#ifdef _DEBUG
	lib.Load(_T("7zd.dll"));
#else
	lib.Load(_T("7z.dll"));
#endif // _DEBUG
	
	SevenZip::SevenZipExtractor extractor(lib, archiveName);
	extractor.ExtractArchive(destination);
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		return PrintUsage();
	}

	try
	{
		switch (argv[1][0])
		{
		case _T('c'):
			return CreateArchive(argc, argv);
		case _T('x'):
			return ExtractArchive(argc, argv);
		case _T('h'):
			return HashFiles(argc, argv);
		default:
			break;
		}
	}
	catch (SevenZip::SevenZipException& ex)
	{
		_tprintf(_T("Error: %s\n"), ex.GetMessage().c_str());
	}

	return PrintUsage();
}
