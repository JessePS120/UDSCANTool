import re

def read_c_macro(header_path : str, macro_name : str) -> str:
    with open(header_path, "r") as f:
        contents = f.read()
    match = re.search(rf"^\s*#define\s+{macro_name}\s+(\S+)", contents, re.MULTILINE)
    if match is None:
        raise ValueError(f"Could not find macro '{macro_name}' in {header_path}")
    return match.group(1)