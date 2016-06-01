#!/bin/sh

set -ux

valgrind --track-origins=yes --leak-check=full ./tests 'a = alloc("a")' 'b = alloc("b"); free(a)' 2>/tmp/out1
if [ $? != 0 ]; then
    echo "ERROR" >&2
    exit 1
fi
valgrind --track-origins=yes --leak-check=full ./tests 'a = alloc("a")' 'b = alloc("b"); free(a)' 'for i = 0, 100 do print(i) end' 2>/tmp/out2
if [ $? != 2 ]; then
    echo "ERROR" >&2
    exit 1
fi

for LOG in /tmp/out1 /tmp/out2; do
    if ! grep -q 'ERROR SUMMARY: 0 errors from ' $LOG; then
        echo "ERROR" >&2
        exit 1
    fi
    if ! grep -q 'Resources: "          "' $LOG; then
        echo "RESOURCES LEFT" >&2
        exit 1
    fi
    if ! grep -q 'in use at exit: 0 bytes in 0 blocks' $LOG; then
        echo "LEAK" >&2
        exit 1
    fi
done
