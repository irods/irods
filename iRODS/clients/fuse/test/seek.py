#!/usr/bin/python
import os;
dir='/tmp/mnt'

fn = dir + '/test1.txt';
f=open(fn, 'w+');
s1 = '0123456789';
s2 = '9876543210';
f.write(s1);
f.write(s2);
f.close();

f=open(fn);

f.seek(10);
r1=f.read(10);
print "10:" + r1;
assert r1==s2;
f.seek(0);
r2=f.read(10);
print "0:" + r2;
assert r2==s1;
f.close();
os.remove(fn);


