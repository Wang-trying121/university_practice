# C++ 命令行通讯录系统 (C++ Command-Line Contacts)

[![Language](https://img.shields.io/badge/Language-C%2B%2B11-blue.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/Build-g%2B%2B-green.svg)]()
[![Warnings](https://img.shields.io/badge/Warnings-Zero-orange.svg)]()

## 📝 项目简介

这是我大一阶段作为 C++ 工程化练习的第一个完整命令行程序。项目完全基于 C++ 标准库（STL），未引入任何第三方依赖。实现了联系人的本地文件持久化存储、增删改查、按分组展示以及按姓名排序等功能。

通过这个项目，我初步建立了**工程化思维**，并强制自己使用严格的编译参数（`-Wall -Wextra -Werror`）来保证代码质量。

## ✨ 核心功能

- **基本操作**：添加联系人（姓名、电话、邮箱、分组）、删除、修改、查询。
- **唯一性校验**：添加联系人时自动检查姓名是否重复。
- **分组与排序**：支持按分组展示所有联系人（使用 `std::set` 提取分组），支持按姓名拼音/字母排序（基于 `std::sort` 和自定义比较函数）。
- **本地持久化**：程序启动时自动加载 `联系人.txt`，退出时自动保存。无文件时自动新建。
- **健壮性设计**：
  - 统一的 `typeIn` 函数处理输入，拒绝空字段，防止文件格式错位。
  - 处理了 `cin` 输入流状态错误（非数字菜单输入）及换行符残留问题，程序不会陷入死循环。

## 🛠️ 编译与运行

本项目在 Windows 环境下使用 TDM-GCC 4.9.2 测试通过。请确保你的环境已安装 g++ 编译器。

**编译命令（严格模式）：**

```bash
g++ -std=c++11 -Wall -Wextra -Werror main.cpp -o contacts