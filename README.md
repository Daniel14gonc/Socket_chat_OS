# LAB7_IA

## Compilación de archivos

### Compilación de cliente

g++ client.cpp project.pb.cc -lprotobuf -lstdc++ -o client -pthread -lcurl

El ejecutable creado puede tener cualquier nombre, no necesariamente client.

### Compilación de servidor

g++ server.cpp project.pb.cc -lprotobuf -lstdc++ -o server -pthread

El ejecutable creado puede tener cualquier nombre, no necesariamente server.

## Ejecución de programas

### Ejecución de cliente

./client username IP PORT

El nombre de usuario va en lugar de username, al igual que la IP del servidor al que se desea conectar y el puerto donde está corriendo este.

### Ejecución de servidor

./server PORT

En PORT se especifica el puerto en el que se desea que se corra el servidor.