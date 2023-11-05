# BotProgram

#### Build program linux
```shell
g++ main.cpp linux_functions.cpp task.cpp -o worker -lstdc++
```

#### Build program windows
```shell
cl.exe main.cpp win_functions.cpp ws2_32.lib
```

#### Run Local Server
```
json-server db.json --routes routes.json --host 0.0.0.0
```