# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布后更新本文件，并升级版本号。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 127] - 2026-08-30 (开发中)

### 新增功能
- KhiCAS 中文化（REQ-KHI-007）：删除法语，lang=0 英语，lang=1 中文
- KhiCAS 语言桥接（REQ-KHI-008）：与 ExistOS config_get_language() 联动
- 更新 keymap.md（补充说明书 SHIFT/ALPHA 功能、各视图按键功能）
- 更新 README fork 信息和版本年份 2026

### Bug 修复
- 安全修复：REQ-SEC-002 CDC ERASEB/PROGP 限制数据区域

---

## [build 126] - 2026-08-30 (已发布)

### 安全修复
- REQ-SEC-001: LL_SWI_GET_CONTEXT 增加地址验证
- REQ-SEC-002: 限制 CDC ERASEB/PROGP 仅允许操作数据区（block>=160）

### Bug 修复（25项）
- OSLoader: CDC 缓冲区越界、VM_Unconscious strcat、msc_disk 边界
- OSLoader: 日志缓冲区溢出、mem_cr 累加重置、赋值嵌入条件判断
- OSLoader: DisplayPutStr 内存泄漏、llapi 上下文保存边界检查
- System: PrintTask 栈 400→1024、VROMLoader 缺 return、getentropy 种子
- System: SystemConfig snprintf、bytes_read 顺序、UICore 修复
- System: SysIRQ 中断 printf 移除、MMU case 补 break
- Libs: jpgViewer/mjpegPlayer sizeof(vrambuf)、memory.c 泄漏

### 移除
- 删除 EMU48 Saturn 模拟器（22个文件，约25000行代码）
- 删除按键收集工具 keytest App

---

## 历史版本 (build 125 及之前)

> 以下版本由上游维护：[ExistOS-Team/ExistOS-For-HP39GII](https://github.com/ExistOS-Team/ExistOS-For-HP39GII)

- **build 125**: 基线建立，全量代码审核（121个问题）
- **build 70**: Emu48、KhiCAS 脚本编辑器、JPEG/MicroPython、崩溃日志
- **build 1**: 初始版本（OSLoader、FreeRTOS、虚拟内存、KhiCAS、USB）
