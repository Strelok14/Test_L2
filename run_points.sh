#!/bin/bash

# Проверка наличия аргументов
if [ "$#" -eq 0 ]; then
    echo "Использование: $(basename "$0") file1 [file2 ...]"
    exit 2
fi

# Собираем все аргументы в одну строку
ARGS="$@"

# Запуск point_parser, вывод сразу передается analyze.py через pipe
if ! ./point_parser $ARGS | python3 analyze.py; then
    exit 1
fi

exit 0