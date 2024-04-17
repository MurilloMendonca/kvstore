mkdir -p build
cc -fPIC -O3 -Iinclude/ -c src/mapper.c -o src/mapper.c.o
cc -fPIC -O3 -Iinclude/ -shared src/mapper.c.o -o build/libmapper.so.0.1.0
ln -sf /home/gengar/codes/KVStore/build/libmapper.so.0.1.0 build/libmapper.so
ln -sf /home/gengar/codes/KVStore/build/libmapper.so.0.1.0 build/libmapper.so.0
gcc -fPIC -O3 -Iinclude/ -I/usr/lib/jvm/java-22-openjdk/include -I/usr/lib/jvm/java-22-openjdk/include/linux -c src/mapper.c -o src/mapper.c.o
gcc -fPIC -O3 -Iinclude/ -I/usr/lib/jvm/java-22-openjdk/include -I/usr/lib/jvm/java-22-openjdk/include/linux -c src/jni_wrapper.c -o src/jni_wrapper.c.o
gcc -fPIC -O3 -Iinclude/ -I/usr/lib/jvm/java-22-openjdk/include -I/usr/lib/jvm/java-22-openjdk/include/linux -shared src/mapper.c.o src/jni_wrapper.c.o -o build/libmapperjni.so
cc -fPIC -O3 -Iinclude/ -c src/mapper.c -o src/mapper.c.o -static
ar rcs build/libmapper.a src/mapper.c.o
g++ -std=c++17 -Iinclude/ -c src/client.cpp -o src/client.cpp.o -static
ar rcs build/libkvclient.a src/client.cpp.o
g++ -std=c++17 -g -Iinclude/ -c src/server.cpp -o src/server.cpp.o -static
g++ -std=c++17 -g -Iinclude/ src/server.cpp.o -o build/server -Lbuild -lmapper -static
rustc -C target-feature=+crt-static src/server.rs -o build/rust-server -L build/ -l static=mapper
cc -DTEST -Iinclude/ -c src/mapper.c -o src/mapper.c.o
cc -DTEST -Iinclude/ src/mapper.c.o -o build/mapperTest -Wl,-rpath=/home/gengar/codes/KVStore/build
g++ -std=c++17 -DTEST -Iinclude/ -c src/client.cpp -o src/client.cpp.o
g++ -std=c++17 -DTEST -Iinclude/ src/client.cpp.o -o build/clientTest -Wl,-rpath=/home/gengar/codes/KVStore/build
g++ -std=c++17 -DTEST -DBENCHMARK -Iinclude/ -c src/client.cpp -o src/client.cpp.o
g++ -std=c++17 -DTEST -DBENCHMARK -Iinclude/ src/client.cpp.o -o build/benchmark -Wl,-rpath=/home/gengar/codes/KVStore/build
javac -d ./build/classes src/MapperWrapper.java src/Main.java
cp -r META-INF/ build/
cp build/libmapperjni.so build/classes/
jar cfm build/Main.jar build/META-INF/MANIFEST.MF -C build/classes .
