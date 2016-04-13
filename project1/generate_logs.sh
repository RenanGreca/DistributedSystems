echo "generate logs for 5 nodes"
./diag -v 5 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log5n.html
./diag -v -s 5 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log5s.html

echo "generate logs for 10 nodes"
./diag -v 10 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log10n.html
./diag -v -s 10 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log10s.html

echo "generate logs for 30 nodes"
./diag -v 30 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log30n.html
./diag -v -s 30 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log30s.html

echo "generate logs for 50 nodes"
./diag -v 50 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log50n.html
./diag -v -s 50 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log50s.html
mv plot.png relatorio/randomdsd.png
./diag -v -s -a 50 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log50a.html
mv plot.png relatorio/adaptivedsd.png

echo "generate logs for 100 nodes"
./diag -v 100 > out.log
cat out.log | aha > log.html
mv log.html relatorio/log100n.html