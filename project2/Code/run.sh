make clean
make

./p sample1.s
./p sample2.s
./p sample3.s
./p sample4.s

./p2 -m 0x00400000:0x00400020 -d sample1.o > sample1.txt
./p2 -m 0x00400000:0x00400020 -d sample2.o > sample2.txt
./p2 -m 0x00400000:0x00400020 -d sample3.o > sample3.txt
./p2 -m 0x00400000:0x00400020 -d sample4.o > sample4.txt
