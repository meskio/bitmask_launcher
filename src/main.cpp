#include <iostream>
#include <vector>
#include <string>

#include <boost/python.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <frameobject.h>

namespace py = boost::python;
namespace fs = boost::filesystem;

int
main(int argc, char** argv)
{
  try {
    fs::path full_path(fs::current_path());

    Py_Initialize();

    Py_SetPythonHome(const_cast<char*>(full_path.string().c_str()));

    py::object main_module = py::import("__main__");
    py::object global = (main_module.attr("__dict__"));

    PySys_SetArgv(argc, argv);

    global["_pwd"] = full_path.string();

    py::exec(
      "import sys\n"
      "sys.path = [_pwd + '/lib',\n"
      "            _pwd + '/apps',\n"
      "            _pwd + '/apps/eip',\n"
      "            _pwd]\n"
      "import os\n"
      "import encodings.idna\n" // we need to make sure this is imported
      "sys.argv.append('--standalone')\n", global, global);

    py::exec_file("apps/launcher.py",
                  global,
                  global);
  } catch (py::error_already_set&) {
    PyErr_PrintEx(0);
    return 1;
  }
  return 0;
}
