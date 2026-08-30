# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布时更新本文件。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 126] - 2026-08-30 (开发中)

### Bug 修复
- OSLoader: 修复 CDC 接收缓冲区 off-by-one（start.c:578）
- OSLoader: 修复 VM_Unconscious 中 strcat 写只读内存（start.c:337）
- OSLoader: 修复 msc_disk.c bufsize=0 越界写入，改用 strnlen
- OSLoader: 修复日志缓冲区读取越界（start.c:961）
- OSLoader: 修复 mem_cr 累加未重置（start.c:109）
- OSLoader: 修复启动逻辑中赋值嵌入条件判断（start.c:216）
- OSLoader: 修复 DisplayPutStr 内存泄漏和 strlen+1 问题
- OSLoader: 修复 llapi.c 上下文保存缓冲区溢出检查，溢出时 isSaved 状态保持一致
- OSLoader: 修复 llapi.c LL_SWI_GET_CONTEXT 缺少地址验证（REQ-SEC-001）
- System: 修复 PrintTask 栈溢出（core/main.c:190，400→1024 words）
- System: 修复 VROMLoader 缺少 return 语句
- System: 修复 emu48_porting.c sizeof(指针) 错误
- System: 修复 getentropy.c 硬编码随机数种子，改用 LLAPI 混合熵源
- System: 修复 SystemConfig.c sprintf 溢出改用 snprintf
- System: 修复 SystemConfig.c f_read 失败时 bytes_read 未初始化
- System: 修复 UICore.h 标题分配少 1 字节
- System: 修复 UICore.h vsprintf 改用 vsnprintf
- System: 修复 UICore.h keyMsg 缺少返回值
- System: 修复 UICore.h uint32_t cx < 0 永假条件
- System: 修复 UICore.h setText strcpy 溢出，使用 realloc 统一堆族
- System: 修复 SysIRQ.c IRQ 中调用 printf 和 MMU case 缺 break
- System: 更新 About 页面 GitHub 地址为 fork 作者，增加 fork 说明
- Libs: 修复 jpgViewer.c / mjpegPlayer.c sizeof(vrambuf) 错误
- Libs: 修复 memory.c:227 memory_load() 内存泄漏

### 待完成
- REQ-SEC-002: USB CDC/MSC 命令 flash 操作范围限制（P0）
- REQ-BUILD-005: CI 供应链安全加固（P0）

---

## [build 125] - 2026-08-30

### 基线建立
- 建立需求基线表（55 条需求）
- 建立需求审核流程
- 完成全量代码审核（121 个问题）
- 建立文档体系（需求表、审核表、变更记录、审核报告）
- 安装编译环境（gcc-arm-none-eabi v10.3, cmake, libcrypto++, libusb）
- 构建工具链（sbtools, sys_signer）
- 编译验证通过（OSLoader.sb + ExistOS.sys）

---

## [build 127] - 计划中

### 新增功能
- REQ-KHI-007: KhiCAS 中文化（删除法语，界面/菜单/提示中文）
- REQ-KHI-008: KhiCAS 语言桥接（与 ExistOS config_get_language() 联动）

---

## [build 128] - 计划中

### 新增功能
- REQ-KHI-009: KhiCAS 中文菜单（翻译~200条菜单项，调整布局）

---

## [build 70] - 历史版本

### 新增功能
- USB 输入镍氢电池充电功能（实验性）
- Emu48 Saturn 模拟器运行 hp39g 固件
- JPEG 图片显示
- MicroPython 最小化实现
- 崩溃日志系统
- 固件升级功能（脱离官方工具）
- 用户界面 GUI 框架
- KhiCAS 脚本编辑器

---

## [build 1] - 初始版本

### 核心功能
- OSLoader 引导加载器
- FreeRTOS 多任务调度
- 虚拟内存管理
- LCD 256 级灰度驱动
- 矩阵键盘驱动
- USB MSC/CDC/HID
- FAT16 文件系统
- KhiCAS 数学引擎
- 调试串口
- RTC 时钟
- 基本电源管理
