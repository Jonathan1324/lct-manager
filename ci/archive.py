import shutil
import sys
from pathlib import Path

def archive(folder_name: str, output_name: str):
    folder = Path(folder_name)
    output = Path(output_name)

    archive_path: str

    if sys.platform == "win32":
        archive_path = shutil.make_archive(str(output), 'zip', root_dir=str(folder))
    else:
        archive_path = shutil.make_archive(str(output), 'gztar', root_dir=str(folder))

    return archive_path