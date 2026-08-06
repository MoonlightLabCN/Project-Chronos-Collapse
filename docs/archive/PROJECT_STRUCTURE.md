# 📁 Project Chronos Collapse - 项目结构

## 完整的目录树

```
Project-Chronos-Collapse/
│
├── 📚 文档目录 (Documentation)
│   ├── README.md                      项目概览和入门指南
│   ├── INDEX.md                       文档导航中心 ⭐
│   ├── EXECUTIVE_SUMMARY.md           执行摘要（管理层）
│   ├── DELIVERY_CHECKLIST.md          交付验收清单
│   ├── SESSION_COMPLETE.md            工作会话完成报告
│   ├── TCCMOD_FORMAT.md               .tccmod 格式规范 ⭐⭐⭐
│   ├── PHASE3_REPORT.md               Phase 3 实现报告 ⭐⭐
│   ├── QUICK_REFERENCE.md             快速参考指南 ⭐⭐
│   ├── WORK_SUMMARY.md                工作总结
│   ├── PROJECT_STATUS.md              项目状态追踪
│   ├── CURRENT_STATUS.md              当前状态快照
│   ├── FINAL_STATUS.md                最终状态报告
│   └── SANDBOX_PROBE.md               Phase 1 报告
│
├── 🔧 TinyCC 核心 (TinyCC Core)
│   ├── tccmod.h                       .tccmod 数据结构定义 (250行)
│   ├── tccmod_loader.c                运行时加载器 (700+行)
│   ├── tccmod_builder.c               TCC 序列化器 (400+行)
│   ├── libtcc.h                       [已修改] API 扩展
│   ├── tccelf.c                       [已修改] 输出路由
│   ├── Makefile                       [已修改] 构建配置
│   ├── arm-thumb-gen.c                ARM Thumb-2 代码生成器
│   ├── arm-thumb-instructions.c       Thumb-2 指令定义
│   └── ... (其他 TCC 文件)
│
├── 🧪 测试工具 (Test Tools)
│   └── win32/
│       ├── create_test_module.c       手动生成测试 .tccmod
│       ├── test_simple_loader.c       加载器功能验证
│       ├── test_compile.c             编译测试源码
│       ├── check_layout.c             结构体布局验证
│       ├── test_tccmod_loader.c       完整加载器测试
│       ├── example_module.c           示例模块源码
│       ├── test_arith.c               算术测试
│       └── test_memory_compile.c      内存编译测试
│
├── 🤖 自动化脚本 (Automation Scripts)
│   └── win32/
│       ├── build_test.bat             主构建脚本 ⭐
│       ├── run_simple_test.bat        简单测试运行
│       ├── run_check.bat              布局检查
│       ├── run_check_clean.bat        清理重建
│       ├── check_preprocess.bat       预处理检查
│       ├── test_tccmod_msvc.ps1       MSVC 测试脚本
│       └── Makefile.tccmod            Loader 构建配置
│
├── 📦 编译产物 (Build Artifacts)
│   └── win32/build/
│       ├── test_module.tccmod         测试模块 (161 bytes) ✅
│       ├── create_test_module.exe     模块生成器
│       ├── test_simple_loader.exe     加载器测试
│       ├── check_layout.exe           布局检查工具
│       └── *.obj                      中间文件
│
├── 🏗️ SandboxProbe (Phase 1)
│   ├── SandboxProbe/                  Phase 1 UWP 项目
│   │   └── ARM/Release/               编译输出
│   │       └── SandboxProbe_ARM_Release_FixedCapability.appx
│   └── [源代码和项目文件]
│
└── 🔄 备份目录 (Backup)
    └── erlend_tcc/                    Erlend 的 TCC 分支备份
```

## 📊 文件统计

### 按类型分类

#### 代码文件 (Code Files)
```
核心实现:
  - tccmod.h              250 行
  - tccmod_loader.c       700+ 行
  - tccmod_builder.c      400+ 行
  小计:                   1,350+ 行

测试工具:
  - 8 个测试程序          500+ 行

脚本:
  - 6 个批处理/PS 脚本    200+ 行

代码总计:                 2,050+ 行
```

#### 文档文件 (Documentation)
```
技术文档:
  - TCCMOD_FORMAT.md      500+ 行
  - PHASE3_REPORT.md      600+ 行
  - QUICK_REFERENCE.md    400+ 行

项目管理:
  - PROJECT_STATUS.md     300+ 行
  - CURRENT_STATUS.md     250+ 行
  - FINAL_STATUS.md       500+ 行
  - SESSION_COMPLETE.md   400+ 行
  - WORK_SUMMARY.md       300+ 行

其他:
  - README.md             150+ 行
  - INDEX.md              200+ 行
  - EXECUTIVE_SUMMARY.md  250+ 行
  - DELIVERY_CHECKLIST.md 300+ 行

文档总计:                 3,650+ 行
```

### 按阶段分类

#### Phase 1: Sandbox Probe
```
SandboxProbe/              UWP 测试项目
SANDBOX_PROBE.md           报告文档
```

#### Phase 2: TinyCC ARM Backend
```
arm-thumb-gen.c            1,706 行
arm-thumb-instructions.c   1,761 行
arm-link.c                 [已存在]
```

#### Phase 3: .tccmod Serializer
```
tccmod.h                   250 行
tccmod_loader.c            700+ 行
tccmod_builder.c           400+ 行
测试工具 (8 个)            500+ 行
自动化脚本 (7 个)          200+ 行
文档 (12 个)               3,650+ 行
```

## 🎯 关键文件说明

### ⭐⭐⭐ 核心文件（必读）

1. **TCCMOD_FORMAT.md**
   - 完整的 .tccmod 文件格式规范
   - 所有数据结构定义
   - 加载流程详解

2. **tccmod.h**
   - C 语言数据结构定义
   - API 函数声明
   - 常量和宏定义

3. **tccmod_loader.c**
   - 运行时加载器实现
   - 内存管理
   - 符号解析和重定位

### ⭐⭐ 重要文件

4. **PHASE3_REPORT.md**
   - 实现细节和技术要点
   - 测试结果
   - 经验总结

5. **QUICK_REFERENCE.md**
   - API 快速参考
   - 使用示例
   - 调试技巧

6. **tccmod_builder.c**
   - TCC 序列化器
   - Section/Symbol/Reloc 提取

### ⭐ 有用文件

7. **INDEX.md**
   - 文档导航中心
   - 按角色分类

8. **build_test.bat**
   - 自动化构建和测试
   - 一键验证

9. **SESSION_COMPLETE.md**
   - 完整工作记录
   - 任务清单

## 🔍 快速查找

### 我想要...

#### 了解项目
```
1. README.md           → 项目概览
2. EXECUTIVE_SUMMARY.md → 高层摘要
3. PROJECT_STATUS.md   → 当前进度
```

#### 开始开发
```
1. QUICK_REFERENCE.md  → 快速上手
2. TCCMOD_FORMAT.md    → 格式规范
3. tccmod.h            → 数据结构
```

#### 运行测试
```
1. cd tinycc/win32
2. build_test.bat      → 自动测试
3. run_simple_test.bat → 手动测试
```

#### 理解实现
```
1. PHASE3_REPORT.md    → 实现报告
2. tccmod_loader.c     → 加载器源码
3. tccmod_builder.c    → 序列化器源码
```

#### 调试问题
```
1. QUICK_REFERENCE.md#常见问题
2. check_layout.exe    → 结构验证
3. test_simple_loader.exe → 加载测试
```

## 📈 项目演进

### 文件增长历史

```
Phase 1 (antigravity):
  + SandboxProbe/           UWP 项目
  + SANDBOX_PROBE.md        报告

Phase 2 (antigravity):
  + arm-thumb-gen.c         Thumb-2 生成器
  + arm-thumb-instructions.c Thumb-2 指令
  + arm-link.c              ARM 链接器

Phase 3 (Claude, 2026-07-26):
  + tccmod.h                数据结构
  + tccmod_loader.c         加载器
  + tccmod_builder.c        序列化器
  + 8 个测试工具
  + 7 个自动化脚本
  + 12 个文档文件
  修改: libtcc.h, tccelf.c, Makefile
```

### 代码行数增长

```
开始 (Phase 1): ~500 行 (SandboxProbe)
Phase 2 完成:   ~4,000 行 (+3,500)
Phase 3 完成:   ~10,000 行 (+6,000)
```

## 🎯 下一步文件

### 待创建（Phase 4）
```
□ Phase3Probe/            UWP 测试应用
  ├── MainPage.xaml       主界面
  ├── TccModIntegration.cpp 加载器集成
  └── Package.appxmanifest  配置文件

□ PHASE4_REPORT.md        Phase 4 报告
□ UWP_INTEGRATION.md      UWP 集成指南
```

### 待创建（Phase 5）
```
□ python_tccmod.c         Python 绑定
□ PYTHON_INTEGRATION.md   Python 集成文档
□ examples/               示例代码
  ├── hello_world.py      Hello World
  └── advanced_usage.py   高级用法
```

## 💾 备份建议

### 重要文件备份
```
高优先级:
  - tinycc/tccmod*.{h,c}
  - *.md (所有文档)
  - tinycc/win32/test_*.c

中优先级:
  - tinycc/win32/*.bat
  - tinycc/win32/build/

低优先级:
  - *.obj, *.exe (可重新生成)
```

### Git 建议
```
.gitignore 应包含:
  - *.obj
  - *.exe
  - *.dll
  - build/
  - .vs/
  - *.user
```

## 📝 维护日志

| 日期 | 变更 | 文件数 |
|------|------|--------|
| 2026-07-26 | Phase 3 完成 | +26 个文件 |

---

**最后更新**: 2026-07-26 22:50 UTC+8  
**文件总数**: 50+ 个  
**代码行数**: 6,000+ 行  
**文档行数**: 3,650+ 行
