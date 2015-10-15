#pragma once
#include "NonCopyable.h"
#include "SevenZipLibrary.h"
#include "SevenCryptLibrary.h"
#include <tuple>

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

#define DECLARE_FILES_HASH(method, cate) std::vector< std::tuple<std::string, TString> >& \
	Files##method(const TCHAR* dir_path, const TCHAR* pattern) \
	{ filesHash(cate, dir_path, pattern); return m_filesHash;}

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
	void filesHash(NHasherCate::HasherCate c, const TCHAR* direct, const TCHAR* pattern);
	void filesHash(NHasherCate::HasherCate c, const std::vector<TString>& files);
	void submitTask(NHasherCate::HasherCate c, const TString& file_path);

	// 由于task为异步调用，会储存在内部queue中，所以task的参数列表中不能出现指针或者引用类型
	void fileHashTask(NHasherCate::HasherCate c, TString file_path);

private:
	static const unsigned int INNER_MEM_SIZE = 1024;
	const SevenZipLibrary& m_seven;
	const SevenCryptLibrary& m_crypt;
	SevenWorkerPool* m_threadpool;
	SimpleMemoryPool* m_mempool;
	CRITICAL_SECTION m_csObj;
	std::vector< std::tuple<std::string, TString> > m_filesHash;
};

} // namespace SevenZip