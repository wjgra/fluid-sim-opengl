Compiles with no warnings with 
'''
g++ src\*.cpp src\*.c -o main.exe -W -Wall -Wextra -pedantic -I "C:\SDL-release-2.26.4\include" -lopengl32 -lglu32 -pthread "SDL2.dll"
'''