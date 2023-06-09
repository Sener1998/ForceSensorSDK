# README.md

适配串口通信的力传感器的SDK。

目前功能较少，仅支持读取受力数据及发送自定义指令。后续功能看心情......

强烈建议将本文档全部读完再运行代码，代码里注释少，看不懂别怪我没提醒你看README。

___

## 下载

https://github.com/Sener1998/ForceSensorSDK.git

___

## 编译

代码支持window和linux平台，要求C++14或更高版本。 

### Linux

cmake标准流程，进入项目目录后，依次执行下面命令

```
mkdir build
cd build
cmake ..
make
```

### Windows

推荐使用Visual Studio IDE，直接打开文件夹，自动配置CMAKE即可。

编译器推荐使用MSVC，其它也可以，不过需要修改CMakeList.txt相关配置。

___

## 使用

### 1. 目录结构

```
ForceSensorSDK
├── CMakeLists.txt
├── README.md
├── examples
│   ├── CMakeLists.txt
│   ├── DynPickDemo.cpp
│   └── SerialPortDemo.cpp
└── include
    ├── DynPick.hpp
    ├── ForceSensor.hpp
    └── SerialPort.hpp
```

- examples

该目录下的文件为示例，建议使用SDK前先看demo是如何使用。

| 文件 | 说明 |
| ---- | ---- |
| DynPickrDemo.cpp | 示范了如何使用DynPick类 |
| SerialPortDemo.cpp | 示范了如何使用本项目的串口类 |

- include

本项目将类的实现与声明写在一个hpp中。将这三个hpp添加到你的项目的include目录下，使用时直接包含头文件即可。

| 文件 | 说明 | 备注 |
| ---- | ---- | ---- |
| DynPick.hpp | DynPick类的声明及实现 | 继承ForceSensor类，根据具体的传感器的特性重写了部分函数 |
| ForceSensor.hpp | ForceSensor类的声明及实现 | 抽象类，不可直接实例，如何使用详细参见DynPick.hpp |
| SerialPort.hpp | SerialPort类的声明及实现 | 参考网上别人的代码编写的，不建议修改该文件，出问题大概率我也不知道怎么改 |

### 2. 可执行文件

#### 2.1 位置

生成的可执行文件位置如下，若不指定编译模式则默认是Release

```
.../ForceSensorSDK/bin/Release/
.../ForceSensorSDK/bin/Debug/
```

#### 2.2 运行

 - Linux
 
 其串口设备的访问权限如下

```
crw-rw---- 1 root  dialout   4,  69 May 27 18:40 ttyS5
```

所以有以下三种方式运行程序，以运行DynPickrDemo为例：

方式1：直接sudo，众生平等

```
sudo ./DynPickrDemo
```

方式2：加入dialout组，以用户sener为例

```
sudo gpasswd -M sener dialout  // 加入dialout组
sudo gpasswd -d sener dialout  // 后悔了可以退出
groups                         // 查看当前用户所在的组
./DynPickrDemo                 // 直接运行
```

方式3：修改设备的访问权限，以/dev/ttyS5为例

```
sudo chmod 666 /dev/ttyS5
./DynPickrDemo
```

- Windows

直接运行exe文件

### 3. 注意

本项目的代码是在Windows下的VS中编写的，github仓中的代码是适配Windows的。

若想在Linux环境中编译并运行则需要先将所有的cpp和hpp文件的编码改为UTF-8。
