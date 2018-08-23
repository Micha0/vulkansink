#include "vfs.h"

namespace {
	static Vfs::VirtualDirectory* g_Root = nullptr;
}

Vfs::VirtualFile::VirtualFile(const VirtualFileSystem& vfs, const std::string& path, char flag)
	: m_Path(path)
	, m_Vfs(vfs)
	, m_Flag(flag)
{
}

std::string Vfs::VirtualFile::Path()
{
	return m_Path;
}

Vfs::VirtualDirectory::VirtualDirectory(const std::string& path, VirtualDirectory* parentDir)
	: m_RootPath(path)
	, m_ParentDir(parentDir)
{
}

std::string Vfs::VirtualDirectory::RootPath()
{
	return m_RootPath;
}

void Vfs::VirtualDirectory::Mount(std::string path, std::shared_ptr<VirtualDirectory> dir)
{
	m_MountPoints[path] = dir;
}

std::vector<std::string> Vfs::VirtualDirectory::List()
{
	std::vector<std::string> fileList;
	for (auto file : m_MountPoints)
	{
		fileList.push_back(file.first);
	}

	return fileList;
}

bool Vfs::VirtualDirectory::Exists(const std::string& directory)
{
	return m_MountPoints.find(directory) != m_MountPoints.end();
}

Vfs::VirtualFileSystem* Vfs::VirtualDirectory::FsForPath(std::string path)
{
	std::string::size_type pos = path.find('/');
	if (pos == std::string::npos)
	{
		return nullptr;
	}

	std::string prefix = path.substr(0, pos);
	std::string postfix = path.substr(pos + 1, path.size() - pos - 1);

	if (VirtualDirectory::Exists(prefix))
	{
		return m_MountPoints[prefix]->FsForPath(postfix);
	}
	else
	{
		return nullptr;
	}
}

Vfs::VirtualFileSystem::VirtualFileSystem()
	: VirtualDirectory("", nullptr)
{
}

Vfs::VirtualFileSystem* Vfs::VirtualFileSystem::FsForPath(std::string path)
{
	VirtualFileSystem* overriddenFs = VirtualDirectory::FsForPath(path);
	if (overriddenFs != nullptr)
	{
		return overriddenFs;
	}

	if (path.find(RootPath()) != std::string::npos)
	{
		return this;
	}
	else
	{
		return nullptr;
	}
}

void Vfs::SetRoot(VirtualDirectory* rootDir)
{
	g_Root = rootDir;
}

Vfs::VirtualDirectory* Vfs::GetRoot()
{
    return g_Root;
}

bool Vfs::Exists(const std::string& path)
{
	if (g_Root)
	{
		VirtualFileSystem* targetFs = g_Root->FsForPath(path);
		if (targetFs != nullptr)
		{
			return targetFs->Exists(path);
		}
	}

	return false;
}

std::vector<std::string> Vfs::List(const std::string& dir, bool recursive)
{
	if (dir.empty())
	{
		return g_Root->List();
	}

	VirtualFileSystem* targetFs = g_Root->FsForPath(dir);
	if (targetFs)
	{
		return targetFs->List();
	}

	return std::vector<std::string>();
}
