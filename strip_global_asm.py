import os
import re

# Matches optional // func_... comment line + multi-line #pragma GLOBAL_ASM(...)
PATTERN = re.compile(
    r"(?:[ \t]*\/\/\s*func_[0-9A-Fa-f]+\s*\r?\n)?[ \t]*#pragma\s+GLOBAL_ASM\s*\([\s\S]*?\)\r?\n?",
    re.MULTILINE,
)


def process_file(filepath):
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Remove all GLOBAL_ASM pragmas and comments
    new_content = PATTERN.sub("", content)

    # Collapse 3+ consecutive newlines into 2 so the file stays clean
    new_content = re.sub(r"\n{3,}", "\n\n", new_content)

    if new_content != content:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"Cleaned: {filepath}")


def main(src_dir="src"):
    for root, _, files in os.walk(src_dir):
        for file in files:
            if file.endswith((".c", ".cpp", ".h", ".hpp")):
                process_file(os.path.join(root, file))


if __name__ == "__main__":
    main()
