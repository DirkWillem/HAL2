import pathlib
import subprocess
import shutil


def format_file(file: pathlib.Path) -> bool:
    full_path = file.absolute()
    dir = full_path.parent

    if (clang_format_path := shutil.which("clang-format")) is None:
        return False

    subprocess.run([str(clang_format_path), "-i", str(file)], cwd=str(dir))
    return True
