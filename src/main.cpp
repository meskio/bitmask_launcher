#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <unistd.h>

#include <boost/python.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <frameobject.h>

namespace py = boost::python;
namespace fs = boost::filesystem;

static const std::string UPDATES_DIR = "updates";
static const std::string TEMP_PREFIX = "__tmp_";

void
removeTemps(const std::string source = ".")
{
  fs::path sourceDir(source);
  fs::directory_iterator end_iter;

  std::vector<fs::path> files;

  if (fs::exists(sourceDir) && fs::is_directory(sourceDir))
  {
    for(fs::directory_iterator dir_iter(sourceDir); dir_iter != end_iter; ++dir_iter)
    {
      if (fs::is_regular_file(dir_iter->status()))
      {
        auto filename = dir_iter->path().filename();
        if (boost::ends_with(filename.string(), TEMP_PREFIX))
        {
          std::cout << "remove " << dir_iter->path() << std::endl;
          fs::remove(dir_iter->path());
        }
      }
      else if (fs::is_directory(dir_iter->status()))
      {
        auto currentPath = dir_iter->path();
        removeTemps(currentPath.string());
      } // Ignore other kind of files for now
    }
  }
}

/**
   Given two directories, it merges them by copying new files and
   directories, and replacing existing files with the ones at the
   destination
 */
void
mergeDirectories(const fs::path &source,
                 const fs::path &dest)
{
  fs::path sourceDir(source);
  fs::directory_iterator end_iter;

  std::vector<fs::path> files;

  if (fs::exists(sourceDir) && fs::is_directory(sourceDir))
  {
    for(fs::directory_iterator dir_iter(sourceDir); dir_iter != end_iter; ++dir_iter)
    {
      if (fs::is_regular_file(dir_iter->status()))
      {
        auto filename = dir_iter->path().filename();
        auto destFilePath = dest / filename;
        std::cout << "copy_file " << dir_iter->path() << " to " << destFilePath << std::endl;
        if (fs::exists(destFilePath))
        {
          // On windows we can't remove, but we can rename and afterwards remove
          auto tempFilePath = dest / fs::path(filename.string() + TEMP_PREFIX);
          //fs::remove(destFilePath);
          fs::rename(destFilePath, tempFilePath);
        }
        copy_file(dir_iter->path(), destFilePath);
      }
      else if (fs::is_directory(dir_iter->status()))
      {
        auto currentPath = dir_iter->path();
        auto pathAtDest = dest / currentPath.filename();
        if (!fs::exists(pathAtDest))
        {
          // This just creates the directory
          copy_directory(currentPath, pathAtDest);
        }
        mergeDirectories(currentPath, pathAtDest);
      } // Ignore other kind of files for now
    }
  }
}

void
updateIfNeeded(fs::path &full_path)
{
  fs::path updatePath(full_path / fs::path(UPDATES_DIR));
  if (fs::exists(updatePath))
  {
    std::cout << "Found updates, merging directories before doing anything..."
              << std::endl;
    mergeDirectories(updatePath, full_path);
    fs::remove_all(updatePath);
  }
  else
  {
    std::cout << "No updates found" << std::endl;
    removeTemps();
  }
}

int
main(int argc, char** argv)
{
  try {
    fs::path full_path(fs::system_complete(argv[0]).parent_path());

    auto pypath = full_path.string() + "/apps/:" + full_path.string() + "/lib/";
    std::cout << pypath << std::endl;
#if not defined _WIN32 && not defined _WIN64
    chdir("lib");
    fs::path fromCore("libQtCore.non-ubuntu");
    fs::path toCore("libQtCore.so.4");
    fs::path fromGui("libQtGui.non-ubuntu");
    fs::path toGui("libQtGui.so.4");
    try {
        auto desk = std::string(getenv("DESKTOP_SESSION"));
        if(boost::starts_with(desk, "ubuntu"))
        {
            fs::remove(toCore);
            fs::remove(toGui);
        } else {
            fs::create_symlink(fromCore, toCore);
            fs::create_symlink(fromGui, toGui);
        }
    } catch(...) {

    }
    chdir("..");

    setenv("PYTHONPATH", pypath.c_str(), 1);
#endif

    Py_SetPythonHome(const_cast<char*>(full_path.string().c_str()));
    const char *prog_name = "leap-client";
    Py_SetProgramName(const_cast<char*>(prog_name));
    Py_Initialize();

    py::object main_module = py::import("__main__");
    py::object global = (main_module.attr("__dict__"));

    PySys_SetArgv(argc, argv);

    global["_pwd"] = full_path.string();

    py::exec(
      "import sys\n"
      "sys.path = [_pwd + '/apps',\n"
      "            _pwd + '/lib',\n"
      "            _pwd + '/apps/eip',\n"
      "            _pwd]\n"
      "import os\n"
      "import encodings.idna\n" // we need to make sure this is imported
      "sys.argv.append('--standalone')\n"
      "sys.argv.append('--debug')\n"
      "if not any(map(lambda x: x.startswith('--logfile') or x.startswith('-l'), sys.argv)):\n"
      "    sys.argv.append('--logfile=bitmask.log')\n", global, global);

    py::exec_file("apps/launcher.py",
                  global,
                  global);
  } catch (py::error_already_set&) {
    PyErr_PrintEx(0);
    return 1;
  }
  return 0;
}
