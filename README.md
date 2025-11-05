# Serial Port Assistant

 ![Qt Version](https://img.shields.io/badge/Qt-6.9.3-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-blue.svg)
![MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

一个功能强大的跨平台串口调试助手，基于 Qt6 开发，提供专业级的串口通信调试功能。支持多平台运行，界面美观，功能全面，是嵌入式开发和串口通信调试的理想工具。

![snippet](https://github.com/Maverick-Pi/serial-port-assistant-qt/blob/main/icons/serial-port-assistant.png)



## ✨ 功能特性

### 🔌 核心功能

- **多平台支持** - 支持 Windows、Linux、macOS
- **串口管理** - 自动检测可用串口，实时监控串口状态变化
- **完整配置** - 支持波特率、数据位、停止位、校验位、流控制等完整串口参数配置
- **多语言支持** - 支持中文和英文界面自动切换

### 📡 数据通信

- **实时收发** - 高效的串口数据发送和接收
- **HEX 模式** - 支持十六进制和文本模式的数据显示与发送
- **定时发送** - 可配置间隔时间的自动发送功能
- **结束符支持** - 支持发送和接收结束符（0x03）
- **新行选项** - 发送时自动添加换行符

### 💾 数据管理

- **多文本发送** - 支持9个可配置的发送项，可单独或批量发送
- **循环发送** - 多文本项的循环发送功能，可配置发送间隔
- **指令集管理** - 保存和载入常用指令集
- **历史记录** - 完整的发送历史记录
- **数据保存** - 接收区数据导出到文件

### 🎨 用户界面

- **现代化界面** - 基于 Qt6 的现代化用户界面
- **布局控制** - 可隐藏历史记录区和多文本面板
- **实时状态** - 状态栏显示收发字节统计和实时时间
- **语法高亮** - 接收区和记录区的彩色时间戳显示



## 🚀 快速开始

### 系统要求

- Qt 6.9.3 或更高版本
- C++17 兼容编译器
- CMake 3.16 或更高版本

### 构建步骤

1. **克隆仓库**

   bash

   ```
   git clone https://github.com/Maverick-Pi/serial-port-assistant-qt.git
   cd serial-port-assistant
   ```

   

2. **配置项目**

   bash

   ```
   mkdir build && cd build
   cmake ..
   ```

   

3. **编译项目**

   bash

   ```
   cmake --build .
   # 或者使用 make（Linux/macOS）
   make
   ```

   

4. **安装（可选）**

   bash

   ```
   cmake --install .
   ```

   

### 运行

编译完成后，在 `build` 目录下运行生成的可执行文件。



## 📖 使用指南

### 基本操作

1. **连接串口**
   - 选择可用串口
   - 配置串口参数（波特率、数据位等）
   - 点击"打开串口"
2. **发送数据**
   - 在主发送区输入数据
   - 选择发送模式（文本/HEX）
   - 点击"发送"按钮或使用定时发送
3. **接收数据**
   - 数据自动显示在接收区
   - 支持 HEX 显示模式
   - 可清空或保存接收内容

### 高级功能

- **多文本发送**：在右侧面板配置多个发送项，支持批量管理和循环发送
- **指令集**：保存常用指令组合，方便快速调用
- **界面定制**：根据需要隐藏历史记录区或多文本面板



## 🏗️ 项目结构

text

```
serial-port-assistant/
├── CMakeLists.txt          # 项目构建配置
├── main.cpp                # 程序入口点
├── serialportassistantwidget.h    # 主窗口头文件
├── serialportassistantwidget.cpp  # 主窗口实现
├── serialportassistantwidget.ui   # 主窗口界面设计
├── appcolors.h             # 应用程序颜色定义
├── appcolors.cpp           # 颜色配置实现
├── resources.qrc           # 资源文件
├── serial-port-assistant_zh_CN.ts 	# 翻译文件
└── serial_port_assistant_en_US.ts 	# 翻译文件
```



## 🔧 技术细节

### 核心组件

- **QSerialPort** - Qt 串口通信组件
- **QTimer** - 精确定时器，用于定时发送和状态更新
- **QTextEdit** - 富文本显示，支持彩色时间戳

### 关键特性

- **实时串口检测** - 500ms 间隔自动检测串口变化
- **高效数据处理** - 使用缓冲区管理接收数据
- **内存管理** - 合理的资源分配和释放



## 🤝 贡献指南

我们欢迎各种形式的贡献！请参考以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 开发环境设置

bash

```
# 安装 Qt6 开发环境
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-serialport-dev

# macOS
brew install qt6

# Windows
# 从 Qt 官网下载 Qt6 安装包
```



## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](https://github.com/Maverick-Pi/serial-port-assistant-qt/blob/main/LICENSE) 文件了解详情。



## 🐛 问题反馈

如果您遇到任何问题或有改进建议，请通过以下方式反馈：

1. 在 [GitHub Issues](https://github.com/Maverick-Pi/serial-port-assistant-qt/issues) 提交问题
2. 详细描述问题现象和复现步骤
3. 提供操作系统和 Qt 版本信息



## 🙏 致谢

感谢以下开源项目：

- [Qt](https://www.qt.io/) - 跨平台应用开发框架
- [CMake](https://cmake.org/) - 跨平台构建系统

------

**注意**: 在使用本软件进行串口通信时，请确保遵守相关硬件设备的通信协议和安全规范。

**如果这个项目对你有帮助，请给个 ⭐️ 支持一下！**
