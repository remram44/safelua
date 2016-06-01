#!/bin/sh

valgrind --track-origins=yes --leak-check=full ./tests 'a = alloc("a")' 'b = alloc("b"); free(a)'
valgrind --track-origins=yes --leak-check=full ./tests 'a = alloc("a")' 'b = alloc("b"); free(a)' 'for i = 0, 100 do print(i) end'
