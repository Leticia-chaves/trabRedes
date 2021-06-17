# trabRedes

Requisitos:  
Cmake  
Compilador (pref gcc(Linux) ou Visual Studio 16 2019(Windows)  
Conan configurado para o compilador  
  
Para compilar ir na pasta do projeto e:  
//Windows build  
// $ mkdir build  
// $ cd build  
// $ conan install .. -s build_type=Debug -s compiler="Visual Studio" -s compiler.runtime=MDd  
// $ cmake .. -G "Visual Studio 16 2019"  
// $ cmake --build . --config Debug  
// $ cd bin  
// $ server.exe  
  
//Linux build  
// $ mkdir build  
// $ cd build  
// $ conan install .. --build=missing  
// $ cmake ..  
// $ cmake --build . --config Debug  
// $ cd bin  
// $ ./server  
