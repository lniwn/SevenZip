#pragma once
#include "NonCopyable.h"
#include "SevenZipLibrary.h"
#include "SevenCryptLibrary.h"
#include <map>
#include <functional>

struct IHasher;

namespace SevenZip
{

namespace NHasherCate
{
enum HasherCate
{
	CRC32 = 0U, // id=1
	SHA1,		// id=513
	SHA256,		// id=10
	CRC64,		// id=4
	BLAKE2sp,	// id=514
	MD5
};
} // namespace NHasherCate

class SevenWorkerPool;
class SimpleMemoryPool;
class SevenZipHasher:public NonCopyable
{
private:

#define DECLARE_FILE_HASH(method, cate) TString File##method(const TCHAR* path) \
	{std::string _src = fileHash(cate, path); return strTotstr(_src);}

#define DECLARE_MEMORY_HASH(method, cate) TString Memory##method(const byte* data, unsigned int length) \
	{std::string _src = memoryHash(cate, data, length); return strTotstr(_src);}

#define DECLARE_FILES_HASH(method, cate) void \
	Files##method(const TCHAR* dir_path, const TCHAR* pattern, std::function<void(const TString&)> result_call) \
	{ filesHash(cate, dir_path, pattern, result_call);}

public:
	SevenZipHasher(const SevenZipLibrary& seven, const SevenCryptLibrary& crypt);
	~SevenZipHasher(void);

	DECLARE_FILE_HASH(Crc32, NHasherCate::CRC32)
	DECLARE_FILE_HASH(Sha1, NHasherCate::SHA1)
	DECLARE_FILE_HASH(Sha256, NHasherCate::SHA256)
	DECLARE_FILE_HASH(Crc64, NHasherCate::CRC64)
	DECLARE_FILE_HASH(Blake2sp, NHasherCate::BLAKE2sp)
	DECLARE_FILE_HASH(Md5, NHasherCate::MD5)

	DECLARE_MEMORY_HASH(Crc32, NHasherCate::CRC32)
	DECLARE_MEMORY_HASH(Sha1, NHasherCate::SHA1)
	DECLARE_MEMORY_HASH(Sha256, NHasherCate::SHA256)
	DECLARE_MEMORY_HASH(Crc64, NHasherCate::CRC64)
	DECLARE_MEMORY_HASH(Blake2sp, NHasherCate::BLAKE2sp)
	DECLARE_MEMORY_HASH(Md5, NHasherCate::MD5)

	DECLARE_FILES_HASH(Crc32, NHasherCate::CRC32)
	DECLARE_FILES_HASH(Sha1, NHasherCate::SHA1)
	DECLARE_FILES_HASH(Sha256, NHasherCate::SHA256)
	DECLARE_FILES_HASH(Crc64, NHasherCate::CRC64)
	DECLARE_FILES_HASH(Blake2sp, NHasherCate::BLAKE2sp)
	DECLARE_FILES_HASH(Md5, NHasherCate::MD5)

private:
	std::string fileHash(NHasherCate::HasherCate c, const TCHAR* file_path);
	std::string memoryHash(NHasherCate::HasherCate c, const void* data, unsigned int length);
	std::string fileMd5(std::ifstream* _file);
	std::string memoryMd5(const void* data, unsigned int length);
	IHasher* getHasher(NHasherCate::HasherCate c);
	unsigned short hashToId(NHasherCate::HasherCate c);
	std::wstring hashToName(NHasherCate::HasherCate c);
	void readFile(std::ifstream* _file, const TCHAR* file_path);
	std::string byteHex(const byte* src, unsigned int src_len);
	TString strTotstr(const std::string&);
	void filesHash(NHasherCate::HasherCate c, const TCHAR* direct, const TCHAR* pattern, const std::function<void(const TString&)>& result_call);
	void filesHash(NHasherCate::HasherCate c, const std::vector<TString>& files, const std::function<void(const TString&)>& result_call);
	void submitTask(NHasherCate::HasherCate c, const TString& file_path, const std::function<void(const TString&)>& result_call);

	// task为异步调用，会储存在内部queue中
	void fileHashTask(NHasherCate::HasherCate c, TString file_path);

private:
	static const unsigned int INNER_MEM_SIZE = 1024;
	const SevenZipLibrary& m_seven;
	const SevenCryptLibrary& m_crypt;
	SevenWorkerPool* m_threadpool;
	SimpleMemoryPool* m_mempool;
	CRITICAL_SECTION m_csObj;
	std::map< TString, std::string> m_filesHash;
};

} // namespace SevenZip