#!/bin/bash

# 检查是否提供了文件名参数
if [ -z "$1" ]; then
    echo "错误：请提供要计算CRC32的文件名以及需要添加的文件名！"
    echo "用法：$0 <文件名> <文件名>"
    exit 1
fi

# 获取文件名参数
filename="$1"
sourcefile="$2"

# 通过管道直接提取CRC32值
crc32=$(./${PATH_TD1601_EVB}/bootimgs/crc32.exe "$filename" | awk '{print $NF}')
hex_crc="${crc32#0x}"
# 将32位CRC32拆分为4个字节（小端格式：低字节在前）
byte1="${hex_crc:6:2}"  # 第1字节（最低有效位）
byte2="${hex_crc:4:2}"  # 第2字节
byte3="${hex_crc:2:2}"  # 第3字节
byte4="${hex_crc:0:2}"  # 第4字节（最高有效位）

# 使用printf输出二进制字节并追加到文件
printf "\\x$byte1\\x$byte2\\x$byte3\\x$byte4" >> "$sourcefile"

echo "CRC32值: $crc32"
