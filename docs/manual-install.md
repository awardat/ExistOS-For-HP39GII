# ExistOS 手动安装（命令行）

本页是 README「[手动安装](../README.md#仅安装)」的完整步骤，适用于 **Windows / Linux 命令行**环境。

## 使用 OS_Loader 和 EDB 安装

您需要准备好：

- 固件：从[此处](https://github.com/awardat/ExistOS-For-HP39GII/releases)（本仓库）下载
  - 请下载 `OSLoader.sb` 和 `ExistOS.sys`
- sb_loader：用于将 OSLoader 载入计算器 RAM，若您的 HP39gii 上没有安装 ExistOS 则需要使用。
  - Windows 用户请从[此处](../../../raw/main/tools/sbtools_win/sb_loader.exe)下载二进制文件；
  - Linux 用户请从[此处](../../../archive/refs/heads/main.zip)下载压缩包并解压，进入 `tools/sbtools/` 目录，用 `make` 编译（无需安装）。
    - 可能需要安装crypto++依赖库才能编译成功，参考 [准备环境](../README.md#准备环境)
    - 您会得到 `sb_loader` 可执行文件。
- EDB（Exist Debug Bridge）：用于刷写固件。
  - Windows 用户请从[此处](../../../raw/main/tools/edb.exe)下载二进制文件；
  - Linux 用户请从[此处](https://github.com/ExistOS-Team/edb-unix/archive/refs/heads/master.zip)下载压缩包并解压，使用如下命令编译：
    - `mkdir build`
    - `cmake -B build`
    - `cmake --build build`
    - 您会得到 `edb` 可执行文件。

请将上述文件置于同一文件夹下，以便操作。

若您的 HP39gii 上没有安装 ExistOS，请先：
1. 卸下计算器的所有电池
2. 按住 `ON/C` 并连接 USB 到电脑
3. 运行 `sbloader OSLoader.sb`
  - 计算器将会启动 ExistOS 引导程序，然后将提示找不到系统（如下图），请不要断开计算器电源（USB），继续下面的步骤。
  - ![OSL Boot](../Image/1.png)

当计算器上已有安装 ExistOS 时：
1. 连接 USB 到电脑
2. 运行 `edb -r -f OSLoader.sb 1408 b`
  - 计算器将重新启动，此步骤会刷入 `OSLoader` 引导程序
3. 运行 `edb -r -f ExistOS.sys 1984`
  - 计算器将重新启动，此步骤会刷入 `ExistOS` 主系统
4. 享受 ExistOS 吧
  - 如果遇到问题，或者有意参与本项目，您可以加入 QQ 群（942419621）。

## 固件安装（编译产物刷写）

#### OSLoader（RAM）

注意：HP39GII 的相关驱动程序请自行安装。

提示：已安装 ExistOS 的计算器不需要执行此步骤，除非，刷坏了……

OSLoader 是引导程序，用于加载 ExistOS 并提供底层 API 和虚拟内存相关功能，使用下面的命令刷入 OSLoader（需要计算器处在刷写模式）。

要刷写 OSLoader，需要先将计算器完全断电（卸下所有电池），按住 `ON/C` 键不放，之后插入 USB 数据线。

Windows 系统下可以查看设备管理器是否出现一个名为 “USB 输入设备” 且 ID 为 066F:3770 的 USB HID 设备

![USBID](Image/0.png)

|系统|安装|
|----|----|
|Windows|`ninja sb_flash`|
|Linux|`make sb_flash`|

#### OSLoader

|系统|安装|
|----|----|
|Windows|`ninja edb_flash_loader`|
|Linux|`make edb_flash_loader`|

刷入 OSLoader 后计算器会自动重启，此时已刷入新的 OSLoader。

#### ExistOS

|系统|安装|
|----|----|
|Windows|`ninja edb_flash_sys`|
|Linux|`make edb_flash_sys`|

刷入 ExistOS 后计算器会自动重启，此时已刷入新的系统并可以使用。

