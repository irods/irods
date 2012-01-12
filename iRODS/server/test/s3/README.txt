tar -xvf libs3-1.4.tar
cd libs3-1.4
make DESTDIR=../irodss3 install
    (this creates the library in iRODS/server/test/s3/irodss3
cd ../irodss3
gcc -o bin/gets3.o -Iinclude -c src/gets3.c
gcc -o bin/puts3.o -Iinclude -c src/puts3.c
gcc -o bin/gets3  bin/gets3.o lib/libs3.so.1
gcc -o bin/puts3  bin/puts3.o lib/libs3.so.1
   (this creates the puts3 and gets3   executables)
   (next we set the user access key and password!!)
   (you can read it from a file if you want, I just wanted it simple)
setenv S3_ACCESS_KEY_ID 16S65XMZ2YQ547W6DGR2
setenv S3_SECRET_ACCESS_KEY  <I WILL GIVE THIS TO YOU SEPARATELY>
   (next we put and get a file and show that there is no difference)
bin/puts3 src/gets3.c nsfdemo/gets3.c
bin/gets3  nsfdemo/gets3.c gets3.c
diff  src/gets3.c  gets3.c
rm gets3.c
  (you can use Mozilla with the "s3fox" plug-in to show that this really 
  happened. 
     Follow the steps in the following link FROM STEP 5:
   http://net.tutsplus.com/misc/use-amazon-s3-firefox-to-serve-static-files/
     (ignore steps 1 - 4)


The account name to be used in s3firefox is:
                ptooby@diceresearch.org
   and the access_key_id is the same as given above for S3_ACCESS_KEY_ID
   and the secret key is the same as that for S3_SECRET_ACCESS_KEY 




