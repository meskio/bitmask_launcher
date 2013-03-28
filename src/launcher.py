import time
import threading

from leap.app import main as leap_client
from thandy.ClientCLI import update as thandy_update


class Thandy(threading.Thread):
    def run(self):
        while True:
            try:
                args = [
                    "--repo=/home/chiiph/Code/leap/repo/",
                    "--debug",
                    "--install",
                    "/bundleinfo/LEAPClient/"
                ]
                thandy_update(args)
            except Exception as e:
                print "ERROR1:", e
            finally:
                time.sleep(60)


if __name__ == "__main__":
    thandy_thread = Thandy()
    thandy_thread.daemon = True
    thandy_thread.start()

    leap_client()
