import os
import platform
import time
import threading

from leap.app import main as leap_client
from leap.common.events import server

from thandy.ClientCLI import update as thandy_update


bundles_per_platform = {
    "Windows" : "/bundleinfo/LEAPClient-win/",
    "Darwin" : "",
    "Linux" : "/bundleinfo/LEAPClient/",
}

class Thandy(threading.Thread):
    def run(self):
        while True:
            try:
                os.environ["THANDY_HOME"] = os.path.join(os.getcwd(),
                                                         "config",
                                                         "thandy")
                os.environ["THP_DB_ROOT"] = os.path.join(os.getcwd(),
                                                         "packages")
                os.environ["THP_INSTALL_ROOT"] = os.path.join(os.getcwd(),
                                                              "updates")
                args = [
                    "--repo=repo/",
                    "--install",
                    bundles_per_platform[platform.system()]
                ]
                thandy_update(args)
            except Exception as e:
                print "ERROR:", e
            finally:
                time.sleep(60)


if __name__ == "__main__":
    server.ensure_server(port=8090)

    thandy_thread = Thandy()
    thandy_thread.daemon = True
    thandy_thread.start()

    leap_client()
