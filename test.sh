# this file is basically from chibicc(https://github.com/rui314/chibicc)
#!/bin/bash
cat <<EOF | g++ -xc -c -o tmp1.o - 
int ret3() { return 3; }
int ret5() { return 5; }
int addx(int x,int y) { return x+y; }
int add(int a,int b,int c,int d,int e) {
    return a+b+c+d+e;
}
EOF
afterexit() {
    rm -f tmp tmp.s tmp1.o
    exit
}
assert() {
    expected="$1"
    input="$2"
    ./build/wizardc "$input" 2 > tmp.s || afterexit
    gcc -static -o tmp tmp.s tmp1.o
    ./tmp
    actual="$?"


    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual OK" 
    else 
        echo "$input => $expected expected ,but got $actual"  
    fi
}

assert 0 "int main(){ return 0;}"
assert 255 "int main(){ return -1;}"
assert 5 "int main() { return (3+2*2)-2;}"
assert 7 "int main() { return ((-1*2+8)/2+4);}"
assert 17 "int main() { return 0x1 + 0x10;}"
assert 19 "int main() { return (0x1*0x10 + 0x3);}"
assert 1 "int main() { return 1 > 0; }"
assert 1 "int main() { return 0 < 1;}"
assert 1 "int main() { return 10 >= (1+1);}"
assert 1 "int main() { return 1 <= 2; }"
assert 1 "int main() { return -1 == 0-1; }"
assert 1 "int main() { return 1 != 2; }"
assert 11 "int main() { return 10+1; }"
assert 16 "int main() { return 15+1; }"
assert 11 "int main() { if(1>0) return 11; }"
assert 12 "int main() { if(1>0) return 12; else return 11; }"
assert 12 "int main() { if(1==0) {return 1+10;} else {return 1+11;} }"
assert 1 "int main() { if(1>0) if(0<2) return 1; else {return 11;} }"
assert 11 "int main() { if(1>0) if(0>2) return 1; else {return 11;} }"
assert 1 "int main() { if(1>0) { if(0<2) return 1;} else {return 11;} }"
assert 3 "int main() { if(1>0) { if(0>2) return 1; else return 3;} else {return 11;}}"
assert 3 "int main() {int a = 0;return a+3;}"
assert 14 "int main() {int a=11;return a+3; }"
assert 12 "int main() {int a=5,b=7;return a+b;}"
assert 5 "int main() {int a=5,b=a;return b;}"
assert 3 "int main() {int a=1,b=2;if(a<b) return a+b;else return a-b;}"
assert 7 "int main(){ return ret3() + 4; }"
assert 8 "int main(){ return ret5() + 3; }"
assert 5 "int main(){ return addx(3,2); }"
assert 6 "int main(){ return addx(3,ret3()); }"
assert 15 "int main(){ return add(1,2,3,4,5); }"
assert 8 "int ret7() { return 7;} int main(){ return 1+ret7(); }"
assert 7 "int main(){int a=6;if(1>0){int a = 4,b=3;return a+b;} return a;}"
assert 1 "int main(){if(1>0) return 1; return 6;}"
afterexit