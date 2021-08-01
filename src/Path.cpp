#include "Path.h"
#include "Setting.h"
#include "Logger.h"
#include "Util.h"

namespace rhythmus
{
  
// ------------------------------------------------------------ class PathCache

PathCache::PathCache() {}

void PathCache::Initialize()
{
  PATH = new PathCache();

  /* Cache system directory hierarchy. */
  PATH->CacheSystemDirectory();
}

void PathCache::CacheSystemDirectory()
{
  /* cache system paths for resource. (not song path) */
  CacheDirectory("themes/");
  CacheDirectory("sound/");
  CacheDirectory("bgm/"); /* for LR2 compatiblity */
}

void PathCache::AddLR2SymLink()
{
  PATH->SetSymbolLink("LR2files/Sound/", "sound/");
  PATH->SetSymbolLink("LR2files/Bgm/", "bgm/");
  PATH->SetSymbolLink("LR2files/Theme/", "themes/");
}

void PathCache::CacheDirectory(const std::string& dir)
{
  // XXX: currently only files are cached.
  // XXX: need to convert to absolute path ...
  // XXX: set maximum depth?
  std::vector<DirItem> diritem;
  GetDirectoryItems(dir, diritem, true);
  for (auto& d : diritem) {
    d.filename = PreprocessPath(dir + d.filename);
    path_cached_.emplace_back(d);
  }
}

void PathCache::MakePathHigherPriority(const std::string& path_)
{
  std::string path = PreprocessPath(path_);
  auto it = path_cached_.begin();
  while (it != path_cached_.end()) {
    if (it->filename == path) break;
    ++it;
  }
  if (it == path_cached_.end()) return;
  std::rotate(path_cached_.begin(), it, path_cached_.end());
}

std::string PathCache::GetPath(const std::string& masked_path,
                               bool &is_found) const
{
  std::string r = ResolveMaskedPath(masked_path);
  is_found = r.empty() == false;
  return r;
}

std::string PathCache::GetPath(const std::string& masked_path) const
{
  bool _;
  return GetPath(masked_path, _);
}

void PathCache::GetAllPaths(const std::string& masked_path, std::vector<std::string> &out) const
{
  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos) {
    for (const auto& path : path_cached_) {
      if (path.filename == masked_path) {
        out.push_back(masked_path);
        break;
      }
    }
    return;
  }

  // do path mask matching for cached paths
  for (const auto& path : path_cached_) {
    if (CheckMasking(path.filename, masked_path)) {
      out.push_back(path.filename);
    }
  }
}

void PathCache::GetDirectories(const std::string& masked_path, std::vector<std::string>& out) const
{
  // XXX: uppercase comparsion for non-case-sensitive?
  for (const auto& path : path_cached_) {
    if (path.is_file == 0 && CheckMasking(path.filename, masked_path)) {
      out.push_back(path.filename);
    }
  }
}

void PathCache::GetDescendantDirectories(const std::string& dirpath, std::vector<std::string>& out) const
{
  for (const auto& path : path_cached_) {
    if (path.is_file == 0 &&
        strncmp(dirpath.c_str(), path.filename.c_str(), dirpath.size()) == 0 &&
        path.filename.find_last_of('/') <= dirpath.size()) {
      out.push_back(path.filename);
    }
  }
}

void PathCache::SetSymbolLink(const std::string& path_from, const std::string& path_to)
{
  path_replacement_[path_from] = path_to;
}

void PathCache::RemoveSymbolLink(const std::string& path_src)
{
  path_replacement_.erase(path_src);
}

std::string PathCache::ResolveMaskedPath(const std::string &masked_path) const
{
  std::string path = PreprocessPath(masked_path);

  // check masking first -- if not, return as it is.
  if (path.find('*') == std::string::npos)
    return path;

  // do path mask matching for cached paths
  for (const auto& path_cached : path_cached_) {
    if (CheckMasking(path_cached.filename, path)) {
      return path_cached.filename;
    }
  }

  // if not found, return empty string
  return std::string();
}

std::string PathCache::PreprocessPath(const std::string &path) const
{
  std::string newpath = path;
  for (size_t i = 0; i < newpath.size(); ++i)
    if (newpath[i] == '\\') newpath[i] = '/';
  if (startsWith(newpath, "./"))
    newpath = newpath.substr(2);
  for (const auto& p : path_replacement_) {
    if (p.first.empty()) continue;
    if (p.first.back() == '/' && strnicmp(
      p.first.c_str(),
      newpath.c_str(), p.first.size()) == 0) {
      // if folder, match starting path.
      return p.second + newpath.substr(p.first.size());
    }
    else if (p.first == newpath) {
      // if file, full path must be matched.
      return p.second;
    }
  }
  return newpath;
}


// ----------------------------------------------------------------- class Path

FilePath::FilePath(const std::string& path) : path_(path), is_valid_(false)
{
  InitalizePath(path);
  if (!is_valid_)
    Logger::Warn("FilePath: cannot find path(%s)", path.c_str());
}

FilePath::FilePath(const std::string& path, bool use_alternative_path) : path_(path), is_valid_(false)
{
  InitalizePath(path);
  if (use_alternative_path && !is_valid_) {
    std::string alterpath;
    auto i = path.find_last_of('.');
    if (i != std::string::npos) {
      alterpath = path.substr(0, i) + ".*";
      InitalizePath(alterpath);
    }
  }
  if (!is_valid_)
    Logger::Warn("FilePath: cannot find path(%s)", path.c_str());
}

void FilePath::InitalizePath(const std::string& path)
{
  // first attempt real file path,
  // then scan global cached path object (PathCache).
  if (IsFile(path)) {
    is_valid_ = true;
  }
  else if (PATH) {
    path_ = PATH->GetPath(path, is_valid_);
  }
}

bool FilePath::valid() const { return is_valid_; }
const std::string& FilePath::get() const { return path_; }
const char* FilePath::get_cstr() const { return path_.c_str(); }

DirPath::DirPath(const std::string& path) : path_(path), is_valid_(false)
{
  // first attempt real file path,
  // then scan global cached path object (PathCache).
  if (IsDirectory(path)) {
    is_valid_ = true;
  }
  else if (PATH) {
    path_ = PATH->GetPath(path);
    is_valid_ = IsDirectory(path_);
  }
}

bool DirPath::valid() const { return is_valid_; }
const std::string& DirPath::get() const { return path_; }
const char* DirPath::get_cstr() const { return path_.c_str(); }


PathCache *PATH = nullptr;

}