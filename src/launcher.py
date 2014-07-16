import os
import shutil
import platform
import time
import threading
import ConfigParser

from leap.bitmask.app import start_app as bitmask_client
from leap.common.events import server, signal
from leap.common.events import events_pb2 as proto

import tuf.client.updater

bundles_per_platform = {
    "Windows": "windows",
    "Darwin": "darwin",
    "Linux": "linux",
}

GENERAL_SECTION = "General"
DELAY_KEY = "updater_delay"


class TUF(threading.Thread):
    def __init__(self, config):
        """
        Initialize the list of mirrors, paths and other TUF dependencies from the config file
        """
        if config.has_section(GENERAL_SECTION) and \
                config.has_option(GENERAL_SECTION, DELAY_KEY):
            self.delay = config.getboolean(GENERAL_SECTION, DELAY_KEY)
        else:
            self.delay = 60

        self._load_mirrors(config)
        if not self.mirrors:
            print "ERROR: No updater mirrors found (missing or not well formed launcher.conf)"

        self.bundle_path = os.getcwd()
        self.source_path = self.bundle_path
        self.dest_path = os.path.join(self.bundle_path, 'tmp')
        self.update_path = os.path.join(self.bundle_path, 'updates')

        threading.Thread.__init__(self)

    def run(self):
        """
        Check for updates periodically
        """
        if not self.mirrors:
            return

        while True:
            try:
                tuf.conf.repository_directory = os.path.join(self.bundle_path, 'repo')

                updater = tuf.client.updater.Updater('leap-updater', self.mirrors)
                updater.refresh()

                targets = updater.all_targets()
                updated_targets = updater.updated_targets(targets, self.source_path)
                for target in updated_targets:
                    updater.download_target(target, self.dest_path)
                    self._set_permissions(target)
                if os.path.isdir(self.dest_path):
                    if os.path.isdir(self.update_path):
                        shutil.rmtree(self.update_path)
                    shutil.move(self.dest_path, self.update_path)
                    signal(proto.UPDATER_NEW_UPDATES,
                           content=", ".join(sorted([f['filepath'] for f in updated_targets])))
                    return
            except Exception as e:
                print "ERROR:", e
            finally:
                time.sleep(self.delay)

    def _load_mirrors(self, config):
        self.mirrors = {}
        for section in config.sections():
            if section[:6] != 'Mirror':
                continue
            url_prefix = config.get(section, 'url_prefix')
            metadata_path = bundles_per_platform[platform.system()] + '/metadata'
            targets_path = bundles_per_platform[platform.system()] + '/targets'
            self.mirrors[section[7:]] = {'url_prefix': url_prefix,
                                         'metadata_path': metadata_path,
                                         'targets_path': targets_path,
                                         'confined_target_dirs': ['']}

    def _set_permissions(self, target):
        file_permisions = int(target["fileinfo"]["custom"]["file_permissions"], 8)
        filepath = target['filepath']
        if filepath[0] == '/':
            filepath = filepath[1:]
        file_path = os.path.join(self.dest_path, filepath)
        os.chmod(file_path, file_permisions)


if __name__ == "__main__":
    server.ensure_server(port=8090)

    config = ConfigParser.ConfigParser()
    config.read("launcher.conf")

    tuf_thread = TUF(config)
    tuf_thread.daemon = True
    tuf_thread.start()

    bitmask_client()
