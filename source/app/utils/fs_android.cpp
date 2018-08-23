#include "utils/fs_android.h"
#include "utils/make_unique.h"
#include "utils/log.h"

Vfs::AndroidFile::AndroidFile(AAsset* handle, const AndroidFileSystem& vfs, const std::string& path)
	: Vfs::VirtualFile(vfs, path, static_cast<char>(FileSystemFlag::Readable))
	, m_Handle(handle)
{
}

int64_t Vfs::AndroidFile::Size()
{
	if (m_Handle == nullptr)
	{
		return 0;
	}

	return AAsset_getLength64(m_Handle);
}

Vfs::AndroidFile::~AndroidFile()
{
	AAsset_close(m_Handle);
}

Vfs::AndroidFileSystem::AndroidFileSystem(AAssetManager* manager)
	: m_Manager(manager)
{}

std::vector<std::string> Vfs::AndroidFileSystem::List()
{
	AAssetDir* assetDir = AAssetManager_openDir(m_Manager, "");
	const char* filename;
	std::vector<std::string> fileList;

	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) 
	{
		fileList.push_back(filename);
	}

	AAssetDir_rewind(assetDir);

	return fileList;
}

std::shared_ptr<Vfs::VirtualFile> Vfs::AndroidFileSystem::Open(const std::string& file)
{
	AAsset *asset = AAssetManager_open(m_Manager, file.c_str(), AASSET_MODE_RANDOM);
	return std::shared_ptr<AndroidFile>(new AndroidFile(asset, *this, file));
}

bool Vfs::AndroidFileSystem::Exists(const std::string& file)
{
	AAsset *asset = AAssetManager_open(m_Manager, file.c_str(), AASSET_MODE_RANDOM);
	const bool exists = asset != nullptr;
	if (exists)
	{
		AAsset_close(asset);
	}
	return exists;
}

void Vfs::AndroidFileSystemSetAsRootJni(JNIEnv *env, jobject assetManagerInstance)
{
	AAssetManager* assetManager = AAssetManager_fromJava(env, assetManagerInstance);
	Vfs::AndroidFileSystem* fs = new Vfs::AndroidFileSystem(assetManager);
	Vfs::SetRoot(fs);
}

extern "C" JNIEXPORT void JNICALL Java_com_example_micha_vulkansink_VulkanActivity_nativeSetAssetManager(JNIEnv* jenv, void* reserved, jobject obj)
{
	LOGI("Setting AssetsManager");
	Vfs::AndroidFileSystemSetAsRootJni(jenv, obj);
}
