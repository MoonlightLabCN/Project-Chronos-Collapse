#!/bin/bash
#
# 使用 curl 推送文件到 Windows Device Portal
# 目标: Lumia 950 XL (192.168.3.18)
#

DEVICE_IP="192.168.3.18"
DEVICE_PORT="50443"
BASE_URL="https://${DEVICE_IP}:${DEVICE_PORT}"

# WDP 凭据（如果设备设置了 PIN）
# USERNAME="Administrator"
# PASSWORD="your_pin_here"

echo "=== WDP 文件推送工具 ==="
echo ""
echo "目标设备: ${DEVICE_IP}:${DEVICE_PORT}"
echo ""

# 测试连接
echo "测试连接..."
if curl -k -s --connect-timeout 5 "${BASE_URL}/api/os/info" > /dev/null 2>&1; then
    echo "✓ 设备连接成功"
else
    echo "✗ 无法连接到设备"
    echo ""
    echo "请确保:"
    echo "  1. Device Portal 已启用"
    echo "  2. 设备 IP 正确"
    echo "  3. 在浏览器中能访问: ${BASE_URL}"
    exit 1
fi

echo ""
echo "准备推送文件..."
echo ""

# 定义目标目录
TARGET_DIR="LocalAppData/Phase3Probe"

# 推送函数
push_file() {
    local LOCAL_FILE=$1
    local REMOTE_NAME=$2
    local DESC=$3

    if [ ! -f "$LOCAL_FILE" ]; then
        echo "✗ 跳过: $DESC (文件不存在)"
        return 1
    fi

    local FILE_SIZE=$(stat -f%z "$LOCAL_FILE" 2>/dev/null || stat -c%s "$LOCAL_FILE" 2>/dev/null)
    echo "推送: $DESC ($FILE_SIZE bytes)"
    echo "  $LOCAL_FILE -> $REMOTE_NAME"

    # WDP API: PUT /api/filesystem/apps/file
    local API_URL="${BASE_URL}/api/filesystem/apps/file?knownfolderid=LocalAppData&filename=${REMOTE_NAME}"

    # 如果需要认证，添加: -u "${USERNAME}:${PASSWORD}"
    if curl -k -X PUT \
        -H "Content-Type: application/octet-stream" \
        --data-binary "@${LOCAL_FILE}" \
        -s -w "%{http_code}" \
        "${API_URL}" | grep -q "200\|201"; then
        echo "  ✓ 上传成功"
        return 0
    else
        echo "  ✗ 上传失败"
        return 1
    fi
}

# 推送文件列表
SUCCESS=0
FAIL=0

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 1. 测试模块
if push_file "tinycc/win32/build/test_module.tccmod" \
             "Phase3Probe/test_module.tccmod" \
             "测试模块"; then
    ((SUCCESS++))
else
    ((FAIL++))
fi
echo ""

# 2. 核心头文件
if push_file "tinycc/tccmod.h" \
             "Phase3Probe/tccmod.h" \
             "核心头文件"; then
    ((SUCCESS++))
else
    ((FAIL++))
fi
echo ""

# 3. 加载器源码
if push_file "tinycc/tccmod_loader.c" \
             "Phase3Probe/tccmod_loader.c" \
             "加载器源码"; then
    ((SUCCESS++))
else
    ((FAIL++))
fi
echo ""

# 4. 格式规范文档
if push_file "TCCMOD_FORMAT.md" \
             "Phase3Probe/TCCMOD_FORMAT.md" \
             "格式规范"; then
    ((SUCCESS++))
else
    ((FAIL++))
fi
echo ""

# 5. 快速参考
if push_file "QUICK_REFERENCE.md" \
             "Phase3Probe/QUICK_REFERENCE.md" \
             "快速参考"; then
    ((SUCCESS++))
else
    ((FAIL++))
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "=== 推送完成 ==="
echo "成功: ${SUCCESS} 个文件"
echo "失败: ${FAIL} 个文件"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "✓ 所有文件推送成功！"
    echo ""
    echo "文件位置: C:\\Data\\Users\\DefaultAccount\\AppData\\Local\\Phase3Probe\\"
    exit 0
else
    echo "⚠ 部分文件推送失败"
    echo ""
    echo "可能的原因:"
    echo "  1. WDP 需要认证 - 取消脚本中的认证注释"
    echo "  2. 目标目录不存在 - 先创建目录"
    echo "  3. 权限不足"
    exit 1
fi
