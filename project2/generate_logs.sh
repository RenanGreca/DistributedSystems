echo "generate logs for nodes"
./mutex 1 1 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log2_1.html

./mutex 1 2 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log2_2.html

./mutex 1 10 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log2_3.html

echo "generate logs for 3 nodes"
./mutex 1 2 3 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log3_1.html

./mutex 1 3 5 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log3_2.html

./mutex 1 20 300 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log3_3.html

cp mutex.c relatorio/mutex.c.txt