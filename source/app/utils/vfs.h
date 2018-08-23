#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Vfs
{
	class VirtualFileSystem;
	class VirtualDirectory;
	class VirtualFile;

    void SetRoot(VirtualDirectory* rootDir);
    VirtualDirectory* GetRoot();

	bool Exists(const std::string& file);
    template <typename T>
	std::shared_ptr<T> Open(const std::string& open);
	std::vector<std::string> List(const std::string& dir, bool recursive);

	enum class FileSystemType
	{
		Posix,
		AndroidAsset,
		None
	};

	enum class FileSystemFlag : char
	{
		Readable = 1,
		Writeable = 2,
		Executable = 4
	};

	class VirtualFile
	{
		friend class VirtualFileSystem;

		std::string m_Path;
		const VirtualFileSystem& m_Vfs;
		char m_Flag;

	protected:
		VirtualFile(const VirtualFileSystem& vfs, const std::string& path, char flag = static_cast<char>(FileSystemFlag::Readable));

	public:
		template <typename T>
		int64_t Write(T&, int N = 1)
		{
			return 0;
		}

		template <typename T>
		int64_t FromBuffer(const std::vector<T>& )
		{
			return 0;
		}

		virtual int64_t Size() = 0;

		std::string Path();
	};

	class VirtualDirectory
	{
		friend class VirtualFileSystem;
		friend bool ::Vfs::Exists(const std::string& file);
//        friend std::shared_ptr<::Vfs::VirtualFile> Open<::Vfs::VirtualFile>(const std::string& open);
		friend std::vector<std::string> List(const std::string& dir, bool recursive);

        std::string m_RootPath;
		VirtualDirectory* m_ParentDir;
		std::map<std::string, std::shared_ptr<VirtualDirectory> > m_MountPoints;
	protected:
		VirtualDirectory(const std::string&, VirtualDirectory* = nullptr);
		std::string RootPath();
	public:
        virtual VirtualFileSystem* FsForPath(std::string path);
		void Mount(std::string path, std::shared_ptr<VirtualDirectory>);
		virtual std::vector<std::string> List();
		virtual bool Exists(const std::string&);
	};

	class VirtualFileSystem : public VirtualDirectory
	{
		friend class VirtualDirectory;

		FileSystemType m_Type;

	protected:
		VirtualFileSystem();
		virtual VirtualFileSystem* FsForPath(std::string path);
	public:
		virtual std::shared_ptr<VirtualFile> Open(const std::string&) = 0;
	};

    template <typename T>
    std::shared_ptr<T> Open(const std::string& path)
    {
        VirtualFileSystem* targetFs = Vfs::GetRoot()->FsForPath(path);
        if (targetFs)
        {
            return std::dynamic_pointer_cast<T>(targetFs->Open(path));
        }

        return std::shared_ptr<T>();
    }
}