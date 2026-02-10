# 将二进制文件嵌入为 C 数组头文件
# 输入参数:
#   INPUT_FILE   - 输入的二进制文件路径
#   OUTPUT_HEADER - 输出的头文件路径
#   VAR_NAME     - C 数组的变量名

if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required")
endif()

if(NOT DEFINED OUTPUT_HEADER)
    message(FATAL_ERROR "OUTPUT_HEADER is required")
endif()

if(NOT DEFINED VAR_NAME)
    message(FATAL_ERROR "VAR_NAME is required")
endif()

# 读取二进制文件
file(READ "${INPUT_FILE}" content HEX)

# 计算文件大小
file(SIZE "${INPUT_FILE}" file_size)

# 将十六进制字符串转换为 C 数组格式
string(LENGTH "${content}" hex_length)
set(array_content "")
set(counter 0)
math(EXPR max_index "${hex_length} - 1")

foreach(i RANGE 0 ${max_index} 2)
    string(SUBSTRING "${content}" ${i} 2 byte)
    
    if(counter GREATER 0)
        string(APPEND array_content ", ")
    endif()
    
    # 每行 12 个字节
    math(EXPR line_pos "${counter} % 12")
    if(line_pos EQUAL 0 AND counter GREATER 0)
        string(APPEND array_content "\n  ")
    endif()
    
    string(APPEND array_content "0x${byte}")
    math(EXPR counter "${counter} + 1")
endforeach()

# 生成头文件内容
set(header_content "// Auto-generated file - do not edit
// Generated from: ${INPUT_FILE}

#ifndef EMBEDDED_${VAR_NAME}_H
#define EMBEDDED_${VAR_NAME}_H

unsigned char ${VAR_NAME}[] = {
  ${array_content}
};
unsigned int ${VAR_NAME}_len = ${file_size};

#endif // EMBEDDED_${VAR_NAME}_H
")

# 写入头文件
file(WRITE "${OUTPUT_HEADER}" "${header_content}")
message(STATUS "Generated embedded resource header: ${OUTPUT_HEADER}")
