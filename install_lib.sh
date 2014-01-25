set -o errexit

mkdir -p lib
cd lib
git clone https://github.com/pmwkaa/sophia.git
cd sophia
make
for file in db/libsophia.*;  do
    sudo cp -v $file /usr/lib
done
sudo cp -v db/sophia.h /usr/include
