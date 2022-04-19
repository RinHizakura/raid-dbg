sudo apt install autoconf autopoint gettext flex bison gawk libcurl4-gnutls-dev
cd elfutils
autoreconf
./configure --enable-maintainer-mode -disable-debuginfod
make
cd ..
mv elfutils/libdw/libdw.so .
