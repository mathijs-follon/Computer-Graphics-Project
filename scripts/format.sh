#!/usr/bin/bash
find ../src/ \( -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.hpp' \) -exec clang-format -i {} +