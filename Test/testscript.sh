# test that reordering doesn't change the formula

mkdir OUT

for x in e*.qcir; do
    echo $x
    ../qubi $x
done

for x in s*.qcir q*.qcir; do
    echo $x
    ../qubi -r -p -k $x > OUT/test.qcir
    diff $x OUT/test.qcir
done

# test that reordering provides the same result

for x in s*.qcir q*.qcir; do
    echo $x
    ../qubi -e -q $x > OUT/test1.txt
    ../qubi -r -e -q $x > OUT/test2.txt
    ../qubi -r -c -e -q $x > OUT/test3.txt
    diff OUT/test1.txt OUT/test2.txt
    diff OUT/test1.txt OUT/test3.txt
done

rm -r OUT