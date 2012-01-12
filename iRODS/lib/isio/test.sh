#!/bin/bash
#
# This script runs the isio test programs in various ways to 
# test the isio library.   It creates and removes various test
# files in the current directory and in irods; so beware.
#
# It also modifies the isio.c source file to have some smaller
# buffersizes to test additional aspects of the caching logic (but
# puts it back before exiting).
#

set -x
set +e

function cleanUpAndDie {
    cp -f src/isio.c.orig src/isio.c
    echo $1
    exit 1
}

function runTests {
  echo "123456789" > t1
  cat t1 t1 t1 t1 t1 t1 t1 t1 t1 t1 t1 t1 t1 t1 > t2

# test1 to irods
  test1 t2 irods:t2
  iget -f t2 t2b
  diff t2 t2b
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 1 failure"
  fi

# small application buffer 
  irm t2
  test1 t2 irods:t2 0 10
  iget -f t2 t2b
  diff t2 t2b
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 2 failure"
  fi

# from irods
  rm -f t2b
  test1 irods:t2 t2b
  diff t2 t2b
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 3 failure"
  fi

# small application buffer
  rm -f t2b
  test1 irods:t2 t2b 0 10
  diff t2 t2b
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 4 failure"
  fi


# larger file
  rm -f t3 t4
  cat src/isio.c src/isio.c src/isio.c src/isio.c src/isio.c  > t3
  cat t3 t3 t3 > t4
  irm -f t4
  test1 t4 irods:t4
  test1 irods:t4 t4b
  diff t4 t4b
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 5 failure"
  fi

# test2 (fgetc/fputc), from irods
  rm -rf t4c
  test2 irods:t4 t4c
  diff t4 t4c
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 6 failure"
  fi

  rm -rf t4d
  test2 irods:t4 t4d 0
  diff t4 t4d
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 7 failure"
  fi

  irm -f t5
  test2 t4d irods:t5
  iget -f t5
  diff t4 t5
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 8 failure"
  fi

# test3; read/write in same file
  rm -f t6 t6a t6b t6c t6d
  touch t6
  test3 t6 0 100 a
  touch t6a
  test3 t6a 0 10 bbbb
  touch t6b
  test3 t6b 0 10 cccc
  cat t6 t6a t6b > t6c
  cat t6 t6a t6a > t6d
  iput -f t6d
  test3 irods:t6d 140 4 cccccccccc
  rm t6d
  iget -f t6d
  diff t6c t6d
  if [ "$?" -ne 0 ]; then
      cleanUpAndDie "test 9 failure"
  fi
}

echo "Echo reduced size initial buffer tests"
cp -f src/isio.c src/isio.c.orig
cat src/isio.c.orig | sed 's/65536/6/g' > t1.c
cp -f t1.c src/isio.c

make
runTests

echo "Echo reduced size initial and max buffer tests"
cat t1.c | sed 's/2097152/20/g' > t2.c
cp -f t2.c src/isio.c

make
runTests

echo "Echo reduced max buffer test"
cat src/isio.c.orig | sed 's/2097152/70000/g' > t3.c
cp -f t3.c src/isio.c

make
runTests

echo "Echo normal size buffer tests"
cp -f src/isio.c.orig src/isio.c
make
runTests

echo "All tests passed"
exit
