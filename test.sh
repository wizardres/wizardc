# this file is basically from chibicc(https://github.com/rui314/chibicc)
#!/bin/bash

afterexit() {
    rm -f tmp tmp.s
    exit
}
assert() {
    expected="$1"
    input="$2"
    ./build/wizardc "$input" 2 > tmp.s || afterexit
    gcc -static -o tmp tmp.s
    ./tmp
    actual="$?"


    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual OK" 
    else 
        echo "$input => $expected expected ,but got $actual"  
    fi
}

assert 0 "0;"
assert 254 "-1;"
assert 5 "(3+2*2)-2;"
assert 7 "((-1*2+8)/2+4);"
assert 1 "1 > 0;"
assert 1 "0 < 1;"
assert 1 "10 >= (1+1);"
assert 1 "1 <= 2;"
assert 1 "-1 == 0-1;"
assert 1 "1 != 2;"

afterexit