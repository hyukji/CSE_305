make clean
make

./p sample1.s
./p sample2.s
./p sample3.s
./p sample4.s

./p3 -antp -m 0x10000000:0x10000020 -d -p sample1.o > sample1_antp.txt
./p3 -atp -m 0x10000000:0x10000020 -d -p sample1.o > sample1_atp.txt

./p3 -antp -m 0x10000000:0x10000020 -d -p sample2.o > sample2_antp.txt
./p3 -atp -m 0x10000000:0x10000020 -d -p sample2.o > sample2_atp.txt

./p3 -antp -m 0x10000000:0x10000020 -d -p sample3.o > sample3_antp.txt
./p3 -atp -m 0x10000000:0x10000020 -d -p sample3.o > sample3_atp.txt

./p3 -antp -m 0x10000000:0x10000020 -d -p sample4.o > sample4_antp.txt
./p3 -atp -m 0x10000000:0x10000020 -d -p sample4.o > sample4_atp.txt

