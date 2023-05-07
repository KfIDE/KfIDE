cmake -DKF_DEBUG:BOOL=true -B.\out -S.\
cd out && cmake --build . && cd ..
.\out\debug\KfIDE.exe