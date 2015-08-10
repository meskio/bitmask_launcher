import os
import os.path
import shutil
import tuf.client.updater
from leap.bitmask.app import start_app as bitmask_client


REPO_DIR = "repo/"
UPDATES_DIR = "updates/"


def update_if_needed():
    if not os.path.isdir(UPDATES_DIR):
        print "No updates found"
        return

    print "Found updates, merging directories before doing anything..."
    try:
        remove_obsolete()
        merge_directories(UPDATES_DIR, ".")
        shutil.rmtree(UPDATES_DIR)
    except Exception as e:
        print "An error has ocurred while updating: " + e.message


def remove_obsolete():
    tuf.conf.repository_directory = REPO_DIR
    updater = tuf.client.updater.Updater('leap-updater', {})
    updater.remove_obsolete_targets(".")


def merge_directories(src, dest):
    for root, dirs, files in os.walk(src):
        if not os.path.exists(root):
            # It was moved as the dir din't exist in dest
            continue

        destroot = os.path.join(dest, root[len(src):])

        for f in files:
            srcpath = os.path.join(root, f)
            destpath = os.path.join(destroot, f)
            if os.path.exists(destpath):
                # FIXME: On windows we can't remove, but we can rename and
                #        afterwards remove. is that still true with python?
                #        or was just something specific of our implementation
                #        with C++?
                os.remove(destpath)
            os.rename(srcpath, destpath)

        for d in dirs:
            srcpath = os.path.join(root, d)
            destpath = os.path.join(destroot, d)

            if os.path.exists(destpath) and not os.path.isdir(destpath):
                os.remove(destpath)

            if not os.path.exists(destpath):
                os.rename(srcpath, destpath)


if __name__ == "__main__":
    update_if_needed()
    bitmask_client()
