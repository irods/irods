# Tips and Tricks

## Confirming checksums

The value reported by `ils -L` for a sha2 checksum is the base64 encoded sha256 checksum.

This can be reproduced with: `sha256sum ${FILENAME} | xxd -r -p | base64`.

Example:
~~~bash
$ ls -l foo
-rw-r--r-- 1 irods irods 423 Feb 24 09:54 foo

$ ils -L foo
  rods              0 demoResc          423 2016-02-24.09:54 & foo
    sha2:zE1sR70ZVlbPawuHeN5fat+xNFVY9RB/cO3gULvPfs8=    generic    /var/lib/irods/Vault/home/rods/foo

$ sha256sum foo
cc4d6c47bd195656cf6b0b8778de5f6adfb1345558f5107f70ede050bbcf7ecf  foo

$ sha256sum foo | xxd -r -p | base64
zE1sR70ZVlbPawuHeN5fat+xNFVY9RB/cO3gULvPfs8=
~~~
