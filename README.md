Minimum Reproducible Example
============================

Build
-----

```
mkdir build
cd build
cmake ..
make
make docker
```

- It is compiled inside the ubuntu 22.04 docker using boost 1.74
- Then mre is executed to create an archive in build/archives/arc


Reproduce
----

```
./mre pack file 10 && ./mre unpack file
./mre unpack archives/arc
```
