#!/bin/bash
FILES=$(find . -type f | grep -P "\./(benchmark|example|include|src|tests)/.*\.(c|cpp|h|hpp|cl|i|js)$")

if [ -n "$1" ]
then
    for file in ${FILES}
    do
        clang-format-15 --dry-run --Werror -i -style=file ${file}
        if [ $? -gt 0 ]; then
            echo "error in file-"${file};
            exit 1;
        fi;
    done
else
    for file in ${FILES}
    do
        clang-format-15 -i -style=file ${file}
    done
fi
