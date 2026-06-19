Build notes:
- No g++ or cl.exe detected in the current terminal environment.
- You may need to install MinGW or use Visual Studio Developer Command Prompt.
- Compile with appropriate command after setting up environment, for example:
  g++ -std=c++17 main.cpp -o othello.exe -lgdi32 -lcomctl32
  or
  cl.exe /EHsc main.cpp user32.lib gdi32.lib comctl32.lib
