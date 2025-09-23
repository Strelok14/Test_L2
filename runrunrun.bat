#!/bin/bash

if [ "$#" -lt 1 ]; then
    echo "Использование: $0 file1 [file2 ...]"
    exit 2
fi

EXITCODE=0

# Запуск C++ -> piped в python
./point_parser.exe "$@" | python3 analyze.py
if [ $? -ne 0 ]; then
    EXITCODE=1
fi

exit $EXITCODE