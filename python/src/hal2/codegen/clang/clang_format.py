import pathlib
import subprocess
import shutil


def format_file(file: pathlib.Path) -> bool:
    """
    Formats the given C/C++ file using ``clang-format``. If ``clang-format`` is not available, does nothing.

    Args:
        file: File to format.

    Returns:
        Whether formatting was successful. If ``clang-format`` is not available, returns ``False``.
    """

    full_path = file.absolute()
    working_directory = full_path.parent

    if (clang_format_path := shutil.which("clang-format")) is None:
        return False

    subprocess.run([str(clang_format_path), "-i", str(file)], cwd=str(working_directory))
    return True
