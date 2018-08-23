#include "vfs.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cmath>

namespace Vfs
{
	void AndroidFileSystemSetAsRootJni(JNIEnv *env, jobject assetManagerInstance);

	class AndroidFileSystem;

	class AndroidFile : public VirtualFile
	{
		friend AndroidFileSystem;

		AAsset* m_Handle;
	protected:
		AndroidFile(AAsset* handle, const AndroidFileSystem& vfs, const std::string& path);

	public:

		template <typename T>
		int64_t Read(T* t, int N = 1)
		{
			if (m_Handle == nullptr)
			{
				return 0;
			}

			N = sizeof(T) * N;

			//TODO: auto chunk on 1MB here, when N > 1024000
			return AAsset_read(m_Handle, t, N);
		}

		template <typename T>
		std::vector<T> ToBuffer()
		{
			if (m_Handle == nullptr)
			{
				return std::vector<T>();
			}

			int requiredSize = (int)ceil((double)Size() / (double)sizeof(T));
			std::vector<T> buffer(requiredSize);
			Read(buffer.data(), requiredSize);
			return buffer;
		}

		virtual int64_t Size();
		virtual ~AndroidFile();
	};

	class AndroidFileSystem : public VirtualFileSystem
	{
		friend void AndroidFileSystemSetAsRootJni(JNIEnv *env, jobject assetManagerInstance);

		AAssetManager* m_Manager;
		AndroidFileSystem(AAssetManager*);
	public:
		virtual std::vector<std::string> List();
		virtual std::shared_ptr<VirtualFile> Open(const std::string& file);
		virtual bool Exists(const std::string& file);
	};
}