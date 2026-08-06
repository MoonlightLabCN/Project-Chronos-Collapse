# 📚 Project Chronos Collapse - 文档索引

欢迎！这是 Project Chronos Collapse 的文档导航中心。

---

## 🎯 快速开始

**新来的？从这里开始：**

1. **[README.md](README.md)** - 项目概览和背景
2. **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** - 快速参考指南
3. **[SESSION_COMPLETE.md](SESSION_COMPLETE.md)** - 当前完成状态

---

## 📖 核心文档

### 技术规范
- **[TCCMOD_FORMAT.md](TCCMOD_FORMAT.md)** ⭐ 必读
  - 完整的 .tccmod 文件格式规范
  - 数据结构定义
  - 加载流程说明
  - 与 TinyCC 的集成方式

### 实现报告
- **[PHASE3_REPORT.md](PHASE3_REPORT.md)** ⭐ 重要
  - Phase 3 完成报告
  - 实现细节和技术要点
  - 测试结果和验证
  - 已知问题和解决方案

### 历史记录
- **[SANDBOX_PROBE.md](SANDBOX_PROBE.md)**
  - Phase 1: JIT 验证报告
  - Lumia 950 XL 测试结果
  - codeGeneration 权限配置

---

## 📊 状态文档

### 项目追踪
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)**
  - 各阶段完成度
  - 任务清单
  - 技术要点回顾

### 当前状态
- **[CURRENT_STATUS.md](CURRENT_STATUS.md)**
  - 最新进展
  - 待完成任务
  - 下一步行动

### 最终报告
- **[FINAL_STATUS.md](FINAL_STATUS.md)**
  - Phase 3 最终状态
  - 完整交付清单
  - 后续路线图

---

## 💼 工作记录

### 会话总结
- **[SESSION_COMPLETE.md](SESSION_COMPLETE.md)** ⭐ 最新
  - 完整的工作会话记录
  - 所有完成的任务
  - 统计数据和成就
  - 给未来开发者的建议

### 工作总结
- **[WORK_SUMMARY.md](WORK_SUMMARY.md)**
  - 4小时工作会话概述
  - 代码量统计
  - 关键成果
  - 经验总结

---

## 🔍 快速导航

### 按角色查找

#### 👨‍💻 开发者
**我想要...**
- 了解项目 → [README.md](README.md)
- 快速上手 → [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
- 理解格式 → [TCCMOD_FORMAT.md](TCCMOD_FORMAT.md)
- 查看代码 → `tinycc/tccmod*.{h,c}`

#### 🧪 测试人员
**我想要...**
- 运行测试 → [QUICK_REFERENCE.md](QUICK_REFERENCE.md#测试)
- 理解测试 → [PHASE3_REPORT.md](PHASE3_REPORT.md#验证结果)
- 查看工具 → `tinycc/win32/test_*.c`

#### 📝 项目经理
**我想要...**
- 查看进度 → [PROJECT_STATUS.md](PROJECT_STATUS.md)
- 了解状态 → [FINAL_STATUS.md](FINAL_STATUS.md)
- 查看统计 → [SESSION_COMPLETE.md](SESSION_COMPLETE.md#统计数据)

#### 🎓 研究人员
**我想要...**
- 技术细节 → [PHASE3_REPORT.md](PHASE3_REPORT.md)
- 设计决策 → [TCCMOD_FORMAT.md](TCCMOD_FORMAT.md)
- 实现原理 → [WORK_SUMMARY.md](WORK_SUMMARY.md#技术亮点)

---

## 📂 文件结构

### 项目根目录
```
Project-Chronos-Collapse/
├── README.md                    项目说明
├── TCCMOD_FORMAT.md            格式规范 ⭐
├── PHASE3_REPORT.md            实现报告 ⭐
├── PROJECT_STATUS.md           项目状态
├── CURRENT_STATUS.md           当前状态
├── FINAL_STATUS.md             最终报告
├── WORK_SUMMARY.md             工作总结
├── QUICK_REFERENCE.md          快速参考 ⭐
├── SESSION_COMPLETE.md         会话完成 ⭐
├── INDEX.md                    本文档
└── SANDBOX_PROBE.md            Phase 1 报告
```

### TinyCC 目录
```
tinycc/
├── tccmod.h                    核心头文件
├── tccmod_loader.c             运行时加载器
├── tccmod_builder.c            TCC 序列化器
├── libtcc.h                    (已修改)
├── tccelf.c                    (已修改)
├── Makefile                    (已修改)
└── win32/
    ├── test_*.c                测试程序
    ├── build_test.bat          构建脚本
    └── build/                  编译输出
```

---

## 🎯 常见任务

### 第一次接触项目
```
1. 阅读 README.md
2. 阅读 QUICK_REFERENCE.md
3. 运行 tinycc/win32/build_test.bat
4. 查看测试输出
```

### 理解实现
```
1. 阅读 TCCMOD_FORMAT.md
2. 阅读 PHASE3_REPORT.md
3. 查看 tccmod.h
4. 查看测试代码
```

### 继续开发
```
1. 查看 CURRENT_STATUS.md
2. 查看 FINAL_STATUS.md
3. 选择待完成任务
4. 参考 QUICK_REFERENCE.md
```

### 调试问题
```
1. 查看 QUICK_REFERENCE.md#常见问题
2. 运行 check_layout.exe
3. 查看 PHASE3_REPORT.md#技术要点
4. 检查结构体定义
```

---

## 📊 文档统计

| 文档 | 大小 | 类型 | 重要性 |
|------|------|------|--------|
| README.md | 中 | 概览 | ⭐⭐⭐ |
| TCCMOD_FORMAT.md | 大 | 规范 | ⭐⭐⭐⭐⭐ |
| PHASE3_REPORT.md | 大 | 报告 | ⭐⭐⭐⭐⭐ |
| QUICK_REFERENCE.md | 中 | 指南 | ⭐⭐⭐⭐⭐ |
| SESSION_COMPLETE.md | 大 | 记录 | ⭐⭐⭐⭐ |
| PROJECT_STATUS.md | 中 | 状态 | ⭐⭐⭐ |
| FINAL_STATUS.md | 大 | 报告 | ⭐⭐⭐⭐ |
| CURRENT_STATUS.md | 中 | 状态 | ⭐⭐⭐ |
| WORK_SUMMARY.md | 中 | 总结 | ⭐⭐⭐ |
| SANDBOX_PROBE.md | 中 | 历史 | ⭐⭐ |

**总文档行数**: ~5,000 行 Markdown

---

## 🔗 外部资源

### TinyCC 相关
- [TinyCC 官网](https://bellard.org/tcc/)
- [TinyCC GitHub](https://github.com/TinyCC/tinycc)
- [Erlend's ARM Thumb-2 分支](链接待补充)

### ARM 架构
- ARM Architecture Reference Manual
- Thumb-2 Instruction Set
- Windows ARM ABI 文档

### Windows 10 Mobile
- UWP API 参考
- Windows.System.Profile.AnalyticsInfo
- VirtualAllocFromApp 文档

---

## 📝 维护日志

| 日期 | 更新内容 | 更新者 |
|------|----------|--------|
| 2026-07-26 | 创建索引文档 | Claude (Opus 5) |
| 2026-07-26 | 完成 Phase 3 所有文档 | Claude (Opus 5) |

---

## 💡 提示

- ⭐ 标记的文档是最重要的
- 所有文档都是 Markdown 格式，可在任何文本编辑器中查看
- 代码示例在 `tinycc/win32/` 目录中
- 测试可以通过运行 `.bat` 脚本自动执行

---

## 🆘 需要帮助？

1. **技术问题** → 查看 [QUICK_REFERENCE.md](QUICK_REFERENCE.md#常见问题)
2. **设计问题** → 查看 [TCCMOD_FORMAT.md](TCCMOD_FORMAT.md)
3. **实现问题** → 查看 [PHASE3_REPORT.md](PHASE3_REPORT.md#技术要点)
4. **测试问题** → 查看 [QUICK_REFERENCE.md](QUICK_REFERENCE.md#调试技巧)

---

**最后更新**: 2026-07-26 22:35 UTC+8  
**维护者**: Project Chronos Collapse Team

---

*感谢使用 Project Chronos Collapse 文档！*
