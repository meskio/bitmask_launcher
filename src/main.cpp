#include <iostream>
#include <vector>
#include <string>

#include <boost/python.hpp>
#include <frameobject.h>

using namespace boost::python;

int
main(int argc, char** argv)
{
  try {
    Py_Initialize();
    object main_module = import("__main__");
    object global = (main_module.attr("__dict__"));

    PySys_SetArgv(argc, argv);

    exec(
      "import os\n"
      "import sys\n"
      "import encodings.idna\n" // we need to make sure this is imported
      "sys.path = [os.path.join(os.getcwd(), 'deps'),\n"
      "            os.path.join(os.getcwd(), 'apps'),\n"
      "            os.path.join(os.getcwd(), 'apps', 'eip'),\n"
      "            os.getcwd()]\n"
      "sys.argv.append('--standalone')\n", global, global);

    exec_file("apps/leap/app.py",
              global,
              global);
  } catch (error_already_set&) {
    PyErr_PrintEx(0);
    return 1;
  }
  return 0;
}
