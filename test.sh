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
assert 255 "-1;"
assert 5 "(3+2*2)-2;"
assert 7 "((-1*2+8)/2+4);"
assert 17 "0x1 + 0x10;"
assert 19 "(0x1*0x10 + 0x3);"
assert 1 "1 > 0;"
assert 1 "0 < 1;"
assert 1 "10 >= (1+1);"
assert 1 "1 <= 2;"
assert 1 "-1 == 0-1;"
assert 1 "1 != 2;"
assert 11 "{10+1;}"
assert 20 "{15+1;12-1+9;}"
assert 11 "if(1>0) 11;"
assert 12 "if(1>0) 12; else 11;"
assert 12 "if(1==0) {1+10;} else {1+11;}"
assert 1 "if(1>0) if(0<2) 1; else {11;}"
assert 11 "if(1>0) if(0>2) 1; else {11;}"
assert 1 "if(1>0) { if(0<2) 1;} else {11;}"
assert 3 "if(1>0) { if(0>2) 1; else 3;} else {11;}"
assert 3 "{int a;a+3;}"
assert 14 "{int a=11;a+3;}"
assert 12 "{int a=5,b=7;a+b;}"
assert 5 "{int a=5,b=a;b;}"
assert 3 "{int a=1,b=2;if(a<b) a+b;else a-b;}"
afterexit