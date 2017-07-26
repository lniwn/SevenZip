#include "StdAfx.h"
#include <7zip/ICoder.h>
#include "SevenZipHasher.h"
#include <fstream>
#include "FileSys.h"
#include <functional>
#include "SevenWorkerPool.h"

namespace SevenZip
{
	static union { char c[4]; unsigned long mylong; } endian_test = {{ 'l', '?', '?', 'b' } };
	#define ENDIANNESS ((char)endian_test.mylong)

SevenZipHasher::SevenZipHasher(const SevenZipLibrary& seven, const SevenCryptLibrary& crypt)
	:m_seven(seven), m_crypt(crypt)
{
	m_threadpool = new SevenWorkerPool;
	m_mempool = new SimpleMemoryPool;

	m_threadpool->Init();
	::InitializeCriticalSectionAndSpinCount(&m_csObj, 5000);
}


SevenZipHasher::~SevenZipHasher(void)
{
	delete m_threadpool;
	delete m_mempool;
	::DeleteCriticalSection(&m_csObj);
}


void SevenZipHasher::readFile(std::ifstream* _file, const TCHAR* file_path)
{
	_file->open(file_path, std::ios::binary);
	if (!_file->good())
	{
		throw SevenZipException(StrFmt(_T("Open %s failed"), file_path));
	}
	//std::ios::pos_type file_length = 0;
	//_file->seekg(0, _file->end);
	//file_length = _file->tellg();
	//_file->seekg(0, _file->beg);
}

void SevenZipHasher::submitTask(NHasherCate::HasherCate c, const TString& file_path, const std::function<void(const TString&)>& result_call)
{
	auto task = std::bind(&SevenZipHasher::fileHashTask, this, c, file_path);
	m_threadpool->SubmitTask(task, std::bind(result_call, file_path));
}


void SevenZipHasher::filesHash(NHasherCate::HasherCate c, const TCHAR* direct, const TCHAR* pattern, const std::function<void(const TString&)>& result_call)
{
	{
		AUTO_SCOPE_LOCK();
		m_filesHash.clear();
	}
	auto files = intl::FileSys::GetFilesInDirectory(direct, pattern, true);

	for (auto it = files.begin(); it != files.end(); ++it)
	{
		submitTask(c, it->FilePath, result_call);
	}
}


void SevenZipHasher::filesHash(NHasherCate::HasherCate c, const std::vector<TString>& files, const std::function<void(const TString&)>& result_call)
{
	{
		AUTO_SCOPE_LOCK();
		m_filesHash.clear();
	}
	
	for (auto it = files.begin(); it != files.end(); ++it)
	{
		submitTask(c, *it, result_call);
	}
}


void SevenZipHasher::fileHashTask(NHasherCate::HasherCate c, TString file_path)
{
	auto hash = fileHash(c, file_path.c_str());
	{
		AUTO_SCOPE_LOCK();
		//m_filesHash.push_back(std::make_tuple(hash, file_path));
		m_filesHash[file_path] = hash;
	}
}

std::string SevenZipHasher::fileHash(NHasherCate::HasherCate c, const TCHAR* file_path)
{
	std::string dst_buf;
	std::ifstream _file;
	readFile(&_file, file_path);
	switch(c)
	{
	case NHasherCate::CRC32:
	case NHasherCate::SHA1:
	case NHasherCate::SHA256:
	case NHasherCate::CRC64:
	case NHasherCate::BLAKE2sp:
		{
			auto read_buf = reinterpret_cast<char*>(m_mempool->Get(INNER_MEM_SIZE));

			IHasher* pHasher = getHasher(c);
			pHasher->Init();

			_file.read(read_buf, INNER_MEM_SIZE);
			std::streamsize readed = 0;
			while (0 != (readed = _file.gcount()))
			{
				pHasher->Update(read_buf, static_cast<UInt32>(readed));
				_file.read(read_buf, INNER_MEM_SIZE);
			}
			m_mempool->Done(read_buf);
			
			//auto digest_buf = std::unique_ptr<byte>(new byte[pHasher->GetDigestSize()], std::default_delete<byte[]>());
			auto digest_buf = reinterpret_cast<byte*>(m_mempool->Get(pHasher->GetDigestSize()));
			pHasher->Final(digest_buf);
			dst_buf = byteHex(digest_buf, pHasher->GetDigestSize());
			pHasher->Release();
			m_mempool->Done(digest_buf);
		}
		break;
	case NHasherCate::MD5:
		{
			return fileMd5(&_file);
		}
		break;
	}
	return dst_buf;
}


std::string SevenZipHasher::fileMd5(std::ifstream* _file)
{
	std::string dst_buf;
	//auto read_buf = std::unique_ptr<byte>(new byte[INNER_MEM_SIZE], std::default_delete<byte[]>());
	auto read_buf = reinterpret_cast<char*>(m_mempool->Get(INNER_MEM_SIZE));
	MD5_CTX ctx;
	m_crypt.MD5Init(&ctx);
	
	_file->read(read_buf, INNER_MEM_SIZE);
	std::streamsize readed = 0;
	while (readed = _file->gcount())
	{
		m_crypt.MD5Update(&ctx, read_buf, static_cast<UInt32>(readed));
		_file->read(read_buf, INNER_MEM_SIZE);
	}
	m_mempool->Done(read_buf);

	m_crypt.MD5Final(&ctx);
	const unsigned int md5_length = 16;

	return byteHex(ctx.digest, md5_length);
}

std::string SevenZipHasher::memoryHash(NHasherCate::HasherCate c, const void* data, unsigned int length)
{
	std::string dst_buf;
	switch(c)
	{
	case NHasherCate::CRC32:
	case NHasherCate::SHA1:
	case NHasherCate::SHA256:
	case NHasherCate::CRC64:
	case NHasherCate::BLAKE2sp:
		{
			IHasher* pHasher = getHasher(c);

			pHasher->Init();
			pHasher->Update(data, length);
			auto digest_len = pHasher->GetDigestSize();
			
			//auto _inner_buf = std::unique_ptr<byte>(new byte[digest_len], std::default_delete<byte[]>());
			auto _inner_buf = reinterpret_cast<byte*>(m_mempool->Get(digest_len));
			pHasher->Final(_inner_buf);
			dst_buf = byteHex(_inner_buf, digest_len);
			pHasher->Release();
			m_mempool->Done(_inner_buf);
		}
		break;
	case NHasherCate::MD5:
		{
			return memoryMd5(data, length);
		}
		break;
	}
	return dst_buf;
}


IHasher* SevenZipHasher::getHasher(NHasherCate::HasherCate c)
{
	IHashers* pHashers;
	m_seven.GetHashers(&pHashers);

	IHasher* pHasher = nullptr;
	PROPVARIANT prop_id;
	auto hr = pHashers->GetHasherProp(c, NMethodPropID::kID, &prop_id);
	if (FAILED(hr))
	{
		pHashers->Release();
		throw SevenZipException(GetCOMErrMsg(_T("memoryHash"), hr));
	}
	PROPVARIANT prop_name;
	hr = pHashers->GetHasherProp(c, NMethodPropID::kName, &prop_name);
	if (FAILED(hr))
	{
		pHashers->Release();
		throw SevenZipException(GetCOMErrMsg(_T("memoryHash"), hr));
	}
	
	auto _id = hashToId(c);
	auto _name = hashToName(c);
	if (_id != prop_id.uiVal || _name.compare(prop_name.bstrVal) != 0)
	{
		pHashers->Release();
		throw SevenZipException(StrFmt(_T("Hasher id %d name %s is mismatching"), _id, _name.c_str()));
	}

	hr = pHashers->CreateHasher(c, &pHasher);
	if (FAILED(hr))
	{
		pHashers->Release();
		throw SevenZipException(GetCOMErrMsg(_T("memoryHash"), hr));
	}
	pHashers->Release();
	return pHasher;
}


std::string SevenZipHasher::memoryMd5(const void* data, unsigned int length)
{
	MD5_CTX ctx;
	m_crypt.MD5Init(&ctx);
	m_crypt.MD5Update(&ctx, data, length);
	m_crypt.MD5Final(&ctx);
	const unsigned int md5_length = 16;
	
	return byteHex(ctx.digest, md5_length);
}


unsigned short SevenZipHasher::hashToId(NHasherCate::HasherCate c)
{
	unsigned short id = -1;
	switch(c)
	{
	case NHasherCate::CRC32:
		id = 1;
		break;
	case NHasherCate::SHA1:
		id = 513;
		break;
	case NHasherCate::SHA256:
		id = 10;
		break;
	case NHasherCate::CRC64:
		id = 4;
		break;
	case NHasherCate::BLAKE2sp:
		id = 514;
		break;
	case NHasherCate::MD5:
		id = -1;
		break;
	}
	return id;
}


std::wstring SevenZipHasher::hashToName(NHasherCate::HasherCate c)
{
	switch(c)
	{
	case NHasherCate::CRC32:
		return L"CRC32";
		break;
	case NHasherCate::SHA1:
		return L"SHA1";
		break;
	case NHasherCate::SHA256:
		return L"SHA256";
		break;
	case NHasherCate::CRC64:
		return L"CRC64";
		break;
	case NHasherCate::BLAKE2sp:
		return L"BLAKE2sp";
		break;
	case NHasherCate::MD5:
		return L"MD5";
		break;
	}
	return L"";
}


std::string SevenZipHasher::byteHex(const byte* src, unsigned int src_len)
{
	std::string hexstr;

	auto innerMem = reinterpret_cast<char*>(m_mempool->Get(INNER_MEM_SIZE));
	switch (src_len)
	{
	case sizeof(UInt32):
		{
			auto result = *reinterpret_cast<const UInt32*>(src);
			sprintf_s (innerMem, INNER_MEM_SIZE, "%08X", result);
		}
		
		break;
	case sizeof(UInt64):
		{
			auto result = *reinterpret_cast<const UInt64*>(src);
			sprintf_s (innerMem, INNER_MEM_SIZE, "%016I64X", result);
		}
		
		break;
	default:
		for (unsigned int i = 0; i < src_len; ++i)
		{
			sprintf_s (&(innerMem[i << 1]), 3, "%02X", src[i]);
		}
		break;
	}

	hexstr = reinterpret_cast<char*>(innerMem);
	m_mempool->Done(innerMem);
	return hexstr;
}


TString SevenZipHasher::strTotstr(const std::string& _src)
{

#ifdef _UNICODE
	auto dst_buf = reinterpret_cast<TCHAR*>(m_mempool->Get(INNER_MEM_SIZE));
	size_t converted; 
	mbstowcs_s(&converted, dst_buf, INNER_MEM_SIZE>>1, _src.c_str(), _src.length());
	m_mempool->Done(dst_buf);
	return dst_buf;
#else
	return _src;
#endif

	
}


} // namespace SevenZip