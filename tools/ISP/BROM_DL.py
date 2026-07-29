import argparse
import time
import sys
from enum import Enum
from pathlib import Path
from hashlib import sha1

import serial
import struct
import serial.tools.list_ports


def configure_output_encoding():
    """Make console output readable in UTF-8 terminals such as Git Bash."""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="backslashreplace")
        except (AttributeError, ValueError):
            pass


configure_output_encoding()


def bundled_resource_path(filename):
    """Return an asset path both from source and a PyInstaller executable."""
    bundle_dir = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent))
    return bundle_dir / filename


def default_imgwriter_path():
    """Prefer imgwriter.bin in the project working directory."""
    project_file = Path.cwd() / "imgwriter.bin"
    return project_file if project_file.is_file() else bundled_resource_path("imgwriter.bin")


def default_firmware_path():
    """Find the project's default image, ignoring macOS AppleDouble metadata."""
    matches = sorted(
        path for path in Path.cwd().glob("*0x18000000_V1.0.0.bin")
        if path.is_file() and not path.name.startswith("._")
    )
    if len(matches) == 1:
        return matches[0]
    if not matches:
        raise FileNotFoundError("project root has no *0x18000000_V1.0.0.bin firmware")
    raise RuntimeError("multiple *0x18000000_V1.0.0.bin firmware files found")

BUFFER_SIZE = 1024  # 缓冲区大小，1024字节
MEM_RW_SPEED = 80  # 单位: k/s

# CRC32多项式常量
CRC32_POLYNOMIAL = 0xEDB88320

# 全局变量
crc_flag = 0


# CRC32表初始化函数
def init_crc_table():
    crc_table = [0] * 256
    for n in range(256):
        c = n
        for k in range(8):
            if c & 1:
                c = CRC32_POLYNOMIAL ^ (c >> 1)
            else:
                c = c >> 1
        crc_table[n] = c
    return crc_table


# 计算CRC32校验值
def calculate_crc32(data, length):
    crc_table = init_crc_table()
    crc = 0xFFFFFFFF

    for i in range(length):
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8)

    return crc ^ 0xFFFFFFFF


# 文件合法性检查
def file_check(file_path):
    global crc_flag

    try:
        with open(file_path, 'rb') as file:
            # 获取文件大小
            file.seek(0, 2)
            file_length = file.tell()
            file.seek(0)

            # 读取整个文件
            file_data = file.read(file_length)

            # 获取CRC标志（从偏移量104处读取4字节）
            if len(file_data) > 108:  # 确保文件足够长
                crc_flag = struct.unpack('<I', file_data[104:108])[0]
                print(f"crc flag is {hex(crc_flag)}")

            # 如果有CRC标志，验证CRC
            if crc_flag:
                # 初始化CRC表并计算
                crc = calculate_crc32(file_data, file_length - 4)
                print(f"crc cal is {hex(crc)}")

                # 从文件末尾4字节读取存储的CRC
                stored_crc = struct.unpack('<I', file_data[file_length - 4:file_length])[0]

                # 比较计算的CRC和存储的CRC
                if crc != stored_crc:
                    print("The file is illegal, it may have been modified")
                    return -1

        return 0
    except Exception as e:
        print(f"File check error: {e}")
        return -1


# 目标设备信息
target_device = {"OutsideSpiFlash-0": {"index": 0, "type": 0x12, "id": 0},
                 "InsideEfuse-0": {"index": 1, "type": 0x16, "id": 0},
                 "InsideSram-0": {"index": 2, "type": 0x2, "id": 0}}


class InitDownloadV2Type(Enum):
    MNTB_IMGWRITER = 0x0
    IMG_IMGWRITER = 0x1
    BIN_FILE = 0x2


class DataSendNsec(Enum):
    DATASEND_NSEC_END = 0x0  # 最后一包下载数据
    DATASEND_NSEC_ONLYONE = 0x1  # 只有一包下载数据，它是开始也是结束
    DATASEND_NSEC_SENDING = 0x2  # 数据正在发送
    DATASEND_NSEC_START = 0x3  # 开始发送数据


class DataSendNsecReply(Enum):
    DATASEND_NESC_SUCCESS = 0x0  # 数据包写入成功
    DATASEND_NSEC_ALL_DONE = 0x1  # 所有数据成功
    DATASEND_NSEC_WRITE_ERROR = 0xE0  # 写入错误
    DATASEND_NSEC_PAKAGE_LENGTH_ERROR = 0xE1  # 数据包长度错误
    DATASEND_NSEC_REQUEST_LENGTH_ERROR = 0xE2  # 请求的数据超过限制


class ConnectNsecReply(Enum):
    CONNECT_NSEC_SUCCESS = 0x1  # 连接请求成功
    CONNECT_NSEC_INIT_ERROR = 0x1  # 初始化时发生错误
    CONNECT_NSEC_FAILURE = 0xE1  # 连接被拒绝
    CONNECT_NSEC_TRY_LATER = 0xE2  # 当前未准备好，请稍后重试


class GetVersionSend(Enum):
    GETVERSION_SOCPROTOCOL = 0x0  # 获取 SoC 支持的协议
    GETVERSION_USERSOCPROTOCOL = 0x1  # 获取用户自定义的 SoC 支持的协议
    GETVERSION_NSEC_PROTOCOL = 0x20  # 获取支持的协议版本


class GetVersionReply(Enum):
    GETVERSION_NSEC_SUCCESS = 0x1  # 获取版本命令执行成功
    GETVERSION_NSEC_NONE_EXIST = 0xE0  # BootROM 中不存在版本信息


class GetDeviceIDSend(Enum):
    GETDEVICEID_ID = 0x0  # 获取设备ID
    GETDEVICEID_NSEC_CPUID = 0x20  # 获取非安全区的CPU ID
    GETDEVICEID_NSEC_MNFCT = 0x21  # 获取非安全区的制造商ID


class GetDeviceIDReply(Enum):
    GETDEVICEIDSUCCESS = 0x1  # 获取设备ID成功
    GETDEVICEIDFAIL = 0x81  # 获取设备ID失败
    GETDEVICEID_NSEC_SUCCESS = 0x1  # 获取非安全区设备ID成功
    GETDEVICEID_NSEC_FAIL = 0xE0  # 获取非安全区设备ID失败


class GetImageTypeSend(Enum):
    GETIMAGETYPE_OTA = 0x0  # 大核image
    GETIMAGETYPE_LC = 0x1  # 小核image


class GetImageTypeReply(Enum):
    GETIMAGETYPE_SUCCESS = 0x1  # 获取Image OTA类型成功
    GETIMAGETYPE_FAILED = 0xE0  # 获取Image OTA类型失败


class GetMemoryInfoReply(Enum):
    GET_MEMORY_INFO_NSEC_SUCCESS = 0x1  # 获取内存信息成功
    GET_MEMORY_INFO_NSEC_FAILURE = 0xE1  # 获取内存信息失败


class GetRunImageReply(Enum):
    RUN_IMAGE_NSEC_SUCCESS = 0x1  # 运行镜像成功
    RUN_IMAGE_NSEC_TARGET_ERROR = 0xE1  # 目标不存在或地址错误
    RUN_IMAGE_NSEC_SOURCE_ERROR = 0xE2  # source can not XIP
    RUN_IMAGE_NSEC_SOURCE_READ_ERROR = 0xE3  # 读取源数据失败


class GetInitDownloadV2Reply(Enum):
    INIT_DOWNLOAD_V2_SUCCESS = 0x1  # 初始化成功
    INIT_DOWNLOAD_V2_LENGTH_INVALID = 0xE0  # 长度无效
    INIT_DOWNLOAD_V2_MEM_DEVICE_NOT_SUPPORT = 0xE1  # 不支持的存储设备
    INIT_DOWNLOAD_V2_MEM_DEVICE_NOT_EXIST = 0xE2  # 存储设备不存在
    INIT_DOWNLOAD_V2_EXCEED_SIZE = 0xE3  # 超过存储设备容量
    INIT_DOWNLOAD_V2_ALIGNMENT_ERR = 0xE4  # 对齐错误
    INIT_DOWNLOAD_V2_IMGWRITER_MNTB_NOT_EXIST = 0xE5  # mntb.imgwriter 不存在


class RunImageStatus(Enum):
    RUN_IMAGE_SEC_SUCCESS = 0x1  # 运行镜像成功
    RUN_IMAGE_SEC_TARGET_ERROR = 0xE1  # 目标不存在或地址错误
    RUN_IMAGE_SEC_XIP_NOT_SUPPORT = 0xE2  # source can not XIP
    RUN_IMAGE_SEC_IMAGE_NOT_EXIST = 0xE3  # 镜像不存在（读取源数据失败）
    RUN_IMAGE_SEC_VERIFY_FAILED = 0xE4  # 校验失败
    RUN_IMAGE_SEC_DEVICE_NOT_SUPPORT = 0xE5  # 存储介质类型错误（在 mntb 中）
    RUN_IMAGE_SEC_OFFSET_NOT_ALIGNED = 0xE6  # 偏移量未按 4 字节对齐（在 mntb 中）
    RUN_IMAGE_SEC_RUN_INFO_ERROR = 0xE7  # mntb 中运行信息数据错误
    RUN_IMAGE_SEC_ERR_PC_IN_IMAGE = 0xE8  # 镜像中的 PC 错误


class HTCMD(Enum):
    # v1
    GET_VERSION = 0x1
    INIT_DOWNLOAD = 0x5
    DOWNLOAD_DATA = 0x6
    GET_TIMEOUT = 0xa
    GET_DEVICE_ID = 0xb

    # v2
    CONNECT = 0x10
    GET_MEMORY_INFO = 0x11
    INIT_DOWNLOAD_NSEC = 0x12
    DOWNLOAD_DATA_NSEC = 0x13
    INIT_DOWNLOAD_V2 = 0x1a
    DOWNLOAD_DATA_V2 = 0x1b
    RESET = 0x1e
    GET_IMAGE_TYPE = 0x1f
    RUN_IMAGE = 0x18


class DownloadType(Enum):
    MANIFEST = 0x1  # 校验文件类型
    IMAGE = 0x2  # 镜像类型


class BootOffset(Enum):
    BOOTA_MNT_OFFSET = 0x10000  # 大核A分区烧录起始位
    BOOTB_MNT_OFFSET = 0x10000  # 大核B分区烧录起始位
    # BOOTB_MNT_OFFSET = 0x100000  # 大核B分区烧录起始位
    LC_BOOTA_MNT_OFFSET = 0x200000  # 小核A分区烧录起始位
    LC_BOOTB_MNT_OFFSET = 0x200000  # 小核A分区烧录起始位
    # LC_BOOTB_MNT_OFFSET = 0x240000  # 小核B分区烧录起始位


from functools import wraps


def measure_execution_time(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        result = func(*args, **kwargs)
        end_time = time.perf_counter()
        elapsed_time = end_time - start_time
        print(f"方法 {func.__name__} 执行时间: {elapsed_time:.4f} 秒")
        return result

    return wrapper


class SerialCommunicator:
    def __init__(self, port_name, baudrate=921600, timeout=1):
        self.port_name = port_name
        self.baudrate = baudrate
        self.timeout = timeout
        self.port = None
        self.connect()

    def connect(self):
        attempts = 0
        while attempts < 3:  # 尝试最多3次连接
            try:
                self.port = serial.Serial(self.port_name, baudrate=self.baudrate, timeout=self.timeout)
                print(f"成功连接到端口: {self.port_name}")
                return  # 连接成功，退出循环
            except serial.SerialException as e:
                attempts += 1
                print(f"串口{self.port_name} 连接失败: {e}. 尝试重新连接... ({attempts}/3)")
                time.sleep(1)  # 等待1秒后重试
            except Exception as e:
                print(f"串口{self.port_name} 连接失败: {e}")
                break  # 其他异常，退出循环

        raise ConnectionError(f"无法连接到端口 {self.port_name}，请检查设备或串口号")

    def parse_cmd(self, cmd, para, length):
        commands = bytearray(4)
        commands[0] = cmd.value
        if para == 0:
            commands[1] = 0x0
        else:
            commands[1] = para.value
        commands[2] = (length & 0xFF00) >> 8
        commands[3] = length & 0xFF
        print("待发送命令为: {}".format(self.format_received_data(commands)))

        return commands

    def send_data(self, length, data):
        try:
            send_data = data[:length]
            self.port.write(send_data)
            bytes_written = len(send_data)
            if bytes_written != length:
                print(f"发送数据失败: 预期发送 {length} 字节, 实际发送 {bytes_written} 字节")
                return -1
        except Exception as e:
            print(f"串口写入失败: {e}")
            return -1
        return 0

    def receive_data(self, length):
        buffer = None
        try:
            # 等待直到缓冲区有足够数据
            start_time = time.time()
            while self.port.in_waiting < length:
                if time.time() - start_time > 10:
                    print(f"实际接收的数据头响应为：{self.port.read(self.port.in_waiting)}")
                    raise TimeoutError(f"接收数据头超时: 缓冲区未达到预期长度 {length} 字节")

            data = self.port.read(length)
            if len(data) != length:
                raise IOError(f"接收数据头失败: 预期接收 {length} 字节, 实际接收 {len(data)} 字节")
            if data[3] != 0:
                expected_body_length = data[3]
                start_time = time.time()
                while self.port.in_waiting < expected_body_length:
                    if time.time() - start_time > 10:
                        print(f"实际接收的数据体响应为：{self.port.read(self.port.in_waiting)}")
                        raise TimeoutError(f"接收数据体超时: 缓冲区数据未达到预期长度 {expected_body_length} 字节")
                buffer = self.port.read(data[3])
                if len(buffer) != expected_body_length:
                    raise IOError(f"接收数据体失败: 预期接收 {expected_body_length} 字节, 实际接收 {len(buffer)} 字节")
        except Exception as e:
            print(f"串口读取失败: {e}")
            raise
        return 0, data, buffer

    def format_received_data(self, data):
        return ' '.join(f'0x{byte:02x}' for byte in data)

    def send_cmd(self, cmd):
        cmd_with_terminator = cmd + '\r\n'
        self.port.reset_output_buffer()
        time.sleep(0.1)
        self.port.write(bytes(cmd_with_terminator, 'utf-8'))
        start_time = time.time()
        receive_data = bytearray(b'')
        timeout = self.timeout
        while time.time() - start_time < timeout:
            if self.port and self.port.in_waiting > 0:
                data = self.port.read(self.port.in_waiting)
                receive_data += data
        return receive_data

    def send_cmd_with_response_check(self, cmd, expected_data):
        response = self.send_cmd(cmd)
        if expected_data.encode('utf-8') in response:
            print(f"命令 {cmd} 执行成功")
        else:
            print(f"命令 {cmd} 执行失败，串口输出中没有匹配到预期关键字")

    def flush_input(self):
        # 清空设备的输入缓冲区
        self.port.reset_input_buffer()

    def flush_output(self):
        # 清空设备的输出缓冲区
        self.port.reset_output_buffer()

    def close(self):
        if self.port.is_open:
            self.port.close()


class DataDownloader:
    def __init__(self, communicator, file_path=None, download_type=None):
        self.communicator = communicator
        self.buffer_size = BUFFER_SIZE
        self.file_path = file_path
        self.download_type = download_type
        self.file_size = Path(file_path).stat().st_size  # 烧录文件总长度
        self.file_data = bytes()  # 烧录文件数据
        self.file_sha1_size = 20  # 20字节为烧录文件的SHA1校验值
        self.default_io_lens = 4

    # 计算文件的SHA1值
    def calc_sha1(self, data):
        sha_1 = sha1(data)
        return sha_1.digest()

    def connect_device(self):
        # CCT Connect Device，烧录流程不依赖此函数，仅是同步转码CCT工具相关逻辑代码，目前直接通过SerialCommunicator建立串口连接
        connect = bytearray([HTCMD.CONNECT.value, 0xef, 0x0, 0x3])
        self.communicator.send_data(self.default_io_lens, connect)

        send_length = 3
        send_data_bytes = bytearray([0x4d, 0x43, 0x54, 0x0])
        self.communicator.send_data(send_length, send_data_bytes)

        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray([HTCMD.CONNECT.value, ConnectNsecReply.CONNECT_NSEC_SUCCESS.value, 0x00, 0x00])
        if receive_result == 0:
            print("升级串口连接响应为:", receive_cmd)
        else:
            print("升级串口连接失败")
        if receive_cmd == expected_output:
            print("Connect request accepted.")

    def get_version(self):
        # get_version = bytearray([HTCMD.GET_VERSION.value, 0x20, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.GET_VERSION, GetVersionSend.GETVERSION_NSEC_PROTOCOL, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray(
            [HTCMD.GET_VERSION.value, GetVersionReply.GETVERSION_NSEC_SUCCESS.value, 0x00, 0x05])
        if receive_result == 0:
            print("版本获取响应为:", receive_cmd)
        if receive_cmd == expected_output:
            is_bootrom = 0 if received_data[0] else 1
            image_version = (received_data[1] << 24) + (received_data[2] << 16) + (received_data[3] << 8) + \
                            received_data[4]
            image_type = "BootLoader" if is_bootrom else "ImageWriter"
            print(f"Get Version success.\nImage type is {image_type}, image version is 0x{image_version}")

    def get_cpu_info(self):
        # get_device = bytearray([0xb, 0x20, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.GET_DEVICE_ID, GetDeviceIDSend.GETDEVICEID_NSEC_CPUID, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray([HTCMD.GET_DEVICE_ID.value, GetDeviceIDReply.GETDEVICEIDSUCCESS.value, 0x00, 0x04])
        if receive_result == 0:
            print("获取CPUID的响应为:", receive_cmd)
        if receive_cmd == expected_output:
            cpuid_0 = (received_data[0] << 24) + (received_data[1] << 16) + (received_data[2] << 8) + received_data[3]
            print(f"Get device id success, CPUID_0 is 0x{cpuid_0:X}.")

    def get_manufacture_info(self):
        # get_device = bytearray([0xb, 0x21, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.GET_DEVICE_ID, GetDeviceIDSend.GETDEVICEID_NSEC_MNFCT, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray([HTCMD.GET_DEVICE_ID.value, GetDeviceIDReply.GETDEVICEIDSUCCESS.value, 0x00, 0x04])
        if receive_result == 0:
            print("获取manufacture info的响应为:", receive_cmd)
        if receive_cmd == expected_output:
            print(f"Get device id success, manufacture info is '{received_data.decode('utf-8')}'")

    def get_image_type(self, type):
        """
        根据type类型，获取当前运行的APP镜像分区
        :param type: GETIMAGETYPE_OTA(大核app)，GETIMAGETYPE_LC(小核APP)
        :return: 如果文件前四个字节匹配特定标识符，则返回1（运行分区为B分区），否则返回0（运行分区为A分区）
        """
        # get_img = bytearray([0x1f, 0x0, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.GET_IMAGE_TYPE, type, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray(
            [HTCMD.GET_IMAGE_TYPE.value, GetImageTypeReply.GETIMAGETYPE_SUCCESS.value, 0x0, 0x1])
        if receive_result == 0:
            print("获取镜像运行分区的响应为:", receive_cmd)
        if receive_cmd == expected_output:
            image_type_str = received_data.decode('utf-8')
            if type.name == "GETIMAGETYPE_OTA":
                print(f"Get BC Image Type success: image{image_type_str}")
            elif type.name == "GETIMAGETYPE_LC":
                print(f"Get LC Image Type success: image{image_type_str}")
            if image_type_str == 'b':
                is_imageb = 1
            elif image_type_str == 'a':
                is_imageb = 0
            else:
                raise IOError(f"获取当前运行的APP镜像分区失败,当前运行分区为：image{image_type_str}")
            return is_imageb
        else:
            raise IOError("获取当前运行的APP镜像分区失败,响应头不符合预期")

    @staticmethod
    def is_mnt_file(file_path, is_app_bin=True):
        """
        判断给定的文件是否为有效的bin文件
        :param file_path: 文件路径
        :param is_app_bin: 是否为app bin文件
        :return: 如果文件前四个字节匹配特定标识符，则返回1，否则返回0
        """
        file = Path(file_path)
        # 检查文件是否存在
        if not file.is_file():
            raise FileNotFoundError(f"待烧录的bin文件不存在: {file_path}")

        if is_app_bin:
            # 读取文件的前4个字节
            with open(file_path, "rb") as f:
                header = f.read(4)

            # 判断是否匹配特定标识符
            if not (len(header) == 4 and header[0] == 0x54 and header[1] == 0x72 and header[2] == 0x42 and header[
                3] == 0x74):
                print(f"非有效app bin文件: {file_path}", file=sys.stderr)
                return 0
            return -1

    def init_download_v2(self, download_file_size, image_addr=0):
        if 'IMGWRITER' in self.download_type.name:
            data_lens = 0
            cmd = self.communicator.parse_cmd(HTCMD.INIT_DOWNLOAD_V2, self.download_type, data_lens)
            print("初始化下载命令为:", cmd)
            self.communicator.send_data(4, cmd)
            send_data_bytes = bytearray(data_lens)
            self.communicator.send_data(data_lens, send_data_bytes)
        else:
            data_lens = 10
            cmd = self.communicator.parse_cmd(HTCMD.INIT_DOWNLOAD_V2, self.download_type, data_lens)
            print("初始化下载命令为:", cmd)
            self.communicator.send_data(4, cmd)

            send_data_bytes = bytearray(data_lens)
            send_data_bytes[0] = target_device["OutsideSpiFlash-0"]["type"]
            send_data_bytes[1] = target_device["OutsideSpiFlash-0"]["id"]
            send_data_bytes[2] = image_addr >> 24
            send_data_bytes[3] = (image_addr >> 16) & 0xff
            send_data_bytes[4] = (image_addr >> 8) & 0xff
            send_data_bytes[5] = image_addr & 0xff
            send_data_bytes[6] = download_file_size >> 24
            send_data_bytes[7] = (download_file_size >> 16) & 0xff
            send_data_bytes[8] = (download_file_size >> 8) & 0xff
            send_data_bytes[9] = download_file_size & 0xff
            self.communicator.send_data(data_lens, send_data_bytes)

        receive_result, receive_cmd, _ = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray(
            [HTCMD.INIT_DOWNLOAD_V2.value, GetInitDownloadV2Reply.INIT_DOWNLOAD_V2_SUCCESS.value, 0x00, 0x00])
        if receive_result == 0:
            print("初始化下载命令返回信息为:", receive_cmd)
        if receive_cmd == expected_output:
            print("初始化下载执行完成")
            print("Init download v2 in secure success,try  to download image .")
        else:
            raise Exception("初始化下载失败")

    def download_send(self, mode, buffer, data):
        download_mode = getattr(DataSendNsec, mode)
        if mode == "DATASEND_NSEC_ONLYONE":
            print("下载整个文件")
        elif mode == "DATASEND_NSEC_START":
            print("开始分块下载")
            print(f"下载第一包数据，单包大小为：{buffer} 字节")
        elif mode == "DATASEND_NSEC_SENDING":
            print(f"下载剩余包数据，单包大小为：{buffer} 字节")
        elif mode == "DATASEND_NSEC_END":
            print(f"下载最后一包数据，单包大小为：{buffer} 字节")
        download_cmd = self.communicator.parse_cmd(HTCMD.DOWNLOAD_DATA_V2, download_mode, buffer)
        self.communicator.send_data(self.default_io_lens, download_cmd)
        sent = self.communicator.send_data(buffer, data)
        return sent

    def download_recv(self, sent, mode):
        if sent < 0:
            raise Exception("文件发送出错，请确认返回信息")
        download_mode = getattr(DataSendNsecReply, mode)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = self.communicator.parse_cmd(HTCMD.DOWNLOAD_DATA_V2, download_mode, 0)
        print("预期响应：{}".format(self.communicator.format_received_data(expected_output)))
        if receive_result == 0:
            print(f"实际响应：{receive_cmd}")
        if receive_cmd == expected_output:
            if mode == "DATASEND_NSEC_ALL_DONE":
                print("文件下载完成")
            else:
                print("分块数据下载完成")
        else:
            raise Exception("下载出错，请确认返回信息")

    @measure_execution_time
    def download_data_v2(self, download_file_size, image_data):
        try:
            print(f"待发送文件的数据大小为：{download_file_size} 字节")
            all_data = image_data
            print(f"待发送的总数据大小为：{len(all_data)} 字节")

            # 文件大小小于缓冲区大小
            if download_file_size <= self.buffer_size:
                ret = self.download_send("DATASEND_NSEC_ONLYONE", download_file_size, all_data)
                if ret < 0:
                    raise Exception("文件发送出错，请确认返回信息")
                self.download_recv(ret, "DATASEND_NSEC_ALL_DONE")
            else:
                # 文件大于缓冲区大小
                count = download_file_size // self.buffer_size
                print(f"分块数量为：{count}")
                flag_piece = download_file_size % self.buffer_size
                print(f"分块后剩余数据大小为：{flag_piece} 字节")

                if flag_piece:
                    piece = count + 1
                else:
                    piece = count

                # 分块读取，最后一包剩余数据加上sha1校验值
                for i in range(piece):
                    if i < count:
                        start_idx = i * self.buffer_size
                        end_idx = start_idx + self.buffer_size
                        # 处理第一包数据
                        if i == 0:
                            data = all_data[start_idx:end_idx]
                            ret = self.download_send("DATASEND_NSEC_START", self.buffer_size, data)
                            if ret < 0:
                                raise Exception("第一包数据发送出错，请确认返回信息")
                            self.download_recv(ret, "DATASEND_NESC_SUCCESS")
                            print("处理第一包数据")
                        # 处理完整分块，没有剩余数据的情况
                        elif count == piece and i == count - 1:
                            data = all_data[start_idx:]
                            send_buffer = len(data)
                            ret = self.download_send("DATASEND_NSEC_END", send_buffer, data)
                            if ret < 0:
                                raise Exception("最后一包数据发送出错，请确认返回信息")
                            self.download_recv(ret, "DATASEND_NSEC_ALL_DONE")
                            print(f"处理最后一包完整分块数据:{send_buffer}")
                        else:
                            # 处理整块数据下载
                            data = all_data[start_idx:end_idx]
                            ret = self.download_send("DATASEND_NSEC_SENDING", self.buffer_size, data)
                            if ret < 0:
                                raise Exception("中间数据发送出错，请确认返回信息")
                            self.download_recv(ret, "DATASEND_NESC_SUCCESS")
                            print("处理整块分块数据")
                    else:
                        # 处理分块后，仍有剩余数据的情况
                        data = all_data[download_file_size - flag_piece:]
                        send_buffer = len(data)
                        ret = self.download_send("DATASEND_NSEC_END", send_buffer, data)
                        if ret < 0:
                            raise Exception("最后一包数据发送出错，请确认返回信息")
                        self.download_recv(ret, "DATASEND_NSEC_ALL_DONE")
                        print(f"处理分块后剩余数据:{send_buffer}")
                    print(f"第{i + 1}包数据下载完成,当前进度：({(i + 1) / piece * 100:.2f}%)")
            return 0
        except Exception as e:
            print(f"文件下载过程中出错: {e}")
            raise

    def reset_device(self):
        # send_data_bytes = bytearray([HTCMD.RESET, 0x0, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.RESET, 0, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray([HTCMD.RESET.value, ConnectNsecReply.CONNECT_NSEC_SUCCESS.value, 0x00, 0x00])
        if receive_result == 0:
            print("RESET命令返回信息为:", receive_cmd)
        if receive_cmd == expected_output:
            print("RESET命令执行成功")


class AppDownloader(DataDownloader):
    def __init__(self, communicator, file_path=None, download_type=None):
        super().__init__(communicator, file_path, download_type)
        self.bc_type = GetImageTypeSend.GETIMAGETYPE_OTA
        self.lc_type = GetImageTypeSend.GETIMAGETYPE_LC
        self.send_buffer_len = 0  # app烧录从get_memory_info获取的动态缓冲区大小
        self.app_buffer = []
        self.app_storage_addr = []
        self.app_length = []
        self.image_count = 0
        self.result = 0
        self.image_length_tt = 0
        self.bc_lc_imageb = 0
        # 执行文件校验(仅对app文件操作,bootloader不需要此功能)
        if file_path:
            global crc_flag
            if file_check(file_path) != 0:
                raise ValueError("文件校验失败，文件可能已损坏或被修改")
            self.load_bin_file()
        self.image_buffer = self.file_data

    def get_memory_info(self):
        # get_mem = bytearray([0x11, 0x0, 0x0, 0x0])
        cmd = self.communicator.parse_cmd(HTCMD.GET_MEMORY_INFO, 0, 0)
        self.communicator.send_data(self.default_io_lens, cmd)
        receive_result, receive_cmd, received_data = self.communicator.receive_data(self.default_io_lens)
        expected_output = bytearray(
            [HTCMD.GET_MEMORY_INFO.value, GetMemoryInfoReply.GET_MEMORY_INFO_NSEC_SUCCESS.value, 0x0, 0x1e])
        if receive_result == 0:
            print("获取内存信息响应为:", receive_cmd)
        if receive_cmd == expected_output:
            print("Get memory info success.")

            if received_data is not None and len(received_data) > 0:
                data_length = len(received_data)

                # 检查数据长度是否为6的倍数
                if data_length % 6 != 0:
                    print("Get memory info length failed.")
                    raise ValueError("内存返回数据长度不是6的整数倍")

                # 从buffer[2]到buffer[5]提取send_buffer_len
                if data_length >= 6:
                    self.send_buffer_len = (received_data[2] << 24) | (received_data[3] << 16) | (
                            received_data[4] << 8) | received_data[5]

                    # 检查send_buffer_len是否为4的倍数（除非为0）
                    if (self.send_buffer_len % 4 != 0) and (self.send_buffer_len != 0):
                        print("Get memory info buffer len failed.")
                        raise ValueError("send_buffer_len不是4的整数倍")

                    print(f"Get memory info success. send_buffer_len = {self.send_buffer_len}")

                    # 更新buffer_size为动态获取的值（如果有效）
                    if self.send_buffer_len > 0:
                        self.buffer_size = self.send_buffer_len
                        print(f"更新单包发送数据量大小为 {self.buffer_size} 字节")
                else:
                    raise ValueError("Get memory info failed: 返回的数据长度不足6字节")
            else:
                print("获取内存信息失败: no data received")
                raise ValueError("内存信息获取失败，未接收到有效数据")
        else:
            print("Get memory info failed.")
            raise IOError("内存信息获取失败")

    def load_bin_file(self):
        with open(self.file_path, 'rb') as file:
            # 如果有CRC标志，减去文件末尾的4字节CRC
            if crc_flag:
                self.file_size -= 4
            self.file_data = file.read(self.file_size)

    def get_image_size(self, addr: bytes) -> int:
        """
        根据 addr 中的字节数据计算镜像大小，并加上偏移 0x1000。
        """
        size = (addr[48] | (addr[49] << 8) | (addr[50] << 16) | (addr[51] << 24)) + 0x1000
        return size

    def get_image_storage_addr(self, addr: bytes) -> int:
        """
        根据 addr 中的字节数据计算镜像的存储地址。
        """
        storage_addr = (addr[40] | (addr[41] << 8) | (addr[42] << 16) | (addr[43] << 24))
        return storage_addr

    def storage_addr_is_config_region(self, addr: bytes) -> int:
        """
        判断 addr 中的存储地址是否属于配置区域,如果是配置区域不会下载mnt表
        如果存储地址小于 0x280000 或大于 0x350000，则返回 0，否则返回 1。
        """
        storage_addr = (addr[40] | (addr[41] << 8) | (addr[42] << 16) | (addr[43] << 24))
        # if (storage_addr < 0x280000) or (storage_addr > 0x400000):
        if (storage_addr < 0x0) or (storage_addr > 0x800000):
            return 0
        return 1

    def judge_ota_type(self, mnt_addr: int) -> int:
        """
        根据传入的 mnt_addr 判断 OTA 类型：
          - 如果 mnt_addr 等于 BOOTA_MNT_OFFSET 或 BOOTB_MNT_OFFSET，返回 0 (大核升级)
          - 如果 mnt_addr 等于 LC_BOOTA_MNT_OFFSET 或 LC_BOOTB_MNT_OFFSET，返回 1 (小核升级)
          - 否则返回 -1。
        """
        if mnt_addr == BootOffset.BOOTA_MNT_OFFSET.value or mnt_addr == BootOffset.BOOTB_MNT_OFFSET.value:
            return 0
        elif mnt_addr == BootOffset.LC_BOOTA_MNT_OFFSET.value or mnt_addr == BootOffset.LC_BOOTB_MNT_OFFSET.value:
            return 1
        else:
            return -1

    def download_bin_file_process(self):
        """
        APP烧录流程
        """
        global crc_flag

        # 获取运行分区烧录缓冲区大小
        self.get_memory_info()

        bc_imageb = 1
        lc_imageb = 1
        # 检查是否含有CRC校验码
        if crc_flag:
            print(f"文件包含CRC校验码，CRC标志值为: {hex(crc_flag)}")

        while self.image_length_tt < self.file_size:
            if self.is_mnt_file(self.file_path):
                image_length = self.get_image_size(self.image_buffer)
                storage_addr = self.get_image_storage_addr(self.image_buffer)
                self.app_storage_addr.append(storage_addr)

                if not self.storage_addr_is_config_region(self.image_buffer):
                    storage_addr -= 0x1000
                    self.app_storage_addr[-1] = storage_addr
                    self.app_length.append(image_length + self.file_sha1_size)
                    self.app_buffer.append(bytearray(image_length + self.file_sha1_size))

                    ota_type = self.judge_ota_type(storage_addr)
                    if ota_type == 1:
                        self.bc_lc_imageb = lc_imageb
                        print(f"judge ota type is LC {self.bc_lc_imageb}")
                    else:
                        self.bc_lc_imageb = bc_imageb
                        print(f"judge ota type is BC {self.bc_lc_imageb}")

                    if self.bc_lc_imageb == 0:
                        storage_addr = self.get_image_storage_addr(self.image_buffer[image_length:]) - 0x1000
                        self.app_storage_addr[-1] = storage_addr
                        self.app_buffer[-1][:image_length] = self.image_buffer[image_length:image_length * 2]
                        buffer_hash_value = self.calc_sha1(self.image_buffer[image_length:image_length * 2])
                    else:
                        self.app_buffer[-1][:image_length] = self.image_buffer[:image_length]
                        buffer_hash_value = self.calc_sha1(self.image_buffer[:image_length])

                    self.app_buffer[-1][image_length:] = buffer_hash_value
                    # self.image_length_tt += (image_length * 2)
                    # self.image_buffer = self.image_buffer[image_length * 2:]
                    self.image_length_tt += image_length
                    self.image_buffer = self.image_buffer[image_length:]
                else:
                    self.app_length.append(image_length - 0x1000 + self.file_sha1_size)
                    self.app_buffer.append(bytearray(image_length - 0x1000 + self.file_sha1_size))
                    self.app_buffer[-1][:image_length - 0x1000] = self.image_buffer[0x1000:image_length]
                    buffer_hash_value = self.calc_sha1(self.image_buffer[0x1000:image_length])
                    self.app_buffer[-1][image_length - 0x1000:] = buffer_hash_value
                    self.image_buffer = self.image_buffer[image_length:]
                    self.image_length_tt += image_length

                self.image_count += 1
            else:
                break
        else:
            print("待烧录bin文件解析完成")

        for i in range(self.image_count):
            print(
                f"正在下载 (part {i}, addr: {hex(self.app_storage_addr[i])}, len: {hex(self.app_length[i] - self.file_sha1_size)})")
            # 初始化下载
            self.init_download_v2(self.app_length[i], self.app_storage_addr[i])
            # 下载数据
            self.result = self.download_data_v2(self.app_length[i], self.app_buffer[i])
            if self.result == 0:
                print(f"(part {i}, addr: {hex(self.app_storage_addr[i])}) 下载成功")
            else:
                raise IOError(f"(part {i}, addr: {hex(self.app_storage_addr[i])}) 下载出错")

        self.reset_device()

# 执行APP烧录
def app_burn_execution(download_serial, download_baudrate, app_bin_path):
    communicator = None
    try:
        print("--------------------- 建立文件烧录串口连接----------------------------")
        communicator = SerialCommunicator(download_serial, download_baudrate)
        # 执行烧录
        ota = AppDownloader(communicator, app_bin_path, InitDownloadV2Type.BIN_FILE)
        ota.download_bin_file_process()
    except Exception as e:
        print(f"App烧录过程中出错: {e}")
        raise
    finally:
        if communicator is not None:
            communicator.close()


# ===================== 配置区域 =====================
UART_BAUDRATE = 57600

bin_array = [
0x4a, 0x69, 0x75, 0x43, 0x68, 0x65, 0x6d, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x0c, 0xe0, 0x02, 
0x00, 0x44, 0x69, 0x00, 0x00, 0x44, 0x00, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 
0x84, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00,
]
# ====================================================

def generate_image_header(image_data: bytes) -> bytes:

    header = bytearray(96)
    header[0] = 0x5C
    header[1] = 0x06
    header[2:9] = b'JiuChen'
    header[9] = 0x40
    header[10] = 0x81

    ram_code_src = 0x40
    header[11:15] = ram_code_src.to_bytes(4, byteorder='little')

    ram_code_dest = 0x00000040
    header[15:19] = ram_code_dest.to_bytes(4, byteorder='little')

    ram_code_size = int.from_bytes(image_data[17:21], byteorder='little')
    header[19:23] = ram_code_size.to_bytes(4, byteorder='little')

    code_entry = int.from_bytes(image_data[21:25], byteorder='little')
    header[23:27] = code_entry.to_bytes(4, byteorder='little')

    header[27:29] = b'\x13\x29'
    header[29:31] = b'\x06\x32'
    header[31:33] = b'\x78\x42'
    header[33:50] = b'\x00' * 17

    image_size = int.from_bytes(image_data[48:52], byteorder='little') + 64
    header[50:54] = image_size.to_bytes(4, byteorder='little')

    check_sum = 0
    for i in range(ram_code_size):
        if 64 + i < len(image_data):
            check_sum += image_data[64 + i]
    header[54:58] = check_sum.to_bytes(4, byteorder='little')

    image_checksum = 0xD3F4B5C6
    header[58:62] = image_checksum.to_bytes(4, byteorder='little')
    header[62:65] = b'\x00\x00\x00'

    add_sum = sum(header[2:65])
    header[65] = add_sum & 0xFF
    return bytes(header)

def read_ack(ser: serial.Serial, cmd: int, timeout_ms: int):
    ser.timeout = timeout_ms / 1000
    data = ser.read(3)
    if len(data) == 3 and data[0] == 0x5A and data[1] == cmd:
        return True, data[2]
    return False, 0

def list_serial_ports():
    ports = list(serial.tools.list_ports.comports())
    print("\n=== 可用串口 ===")
    for i, p in enumerate(ports):
        print(f"{i+1}. {p.device}")
    return ports

# ===================== 主流程：100%对齐你的C代码 =====================
def imgwrite_download(port, file_path):
    try:
        ser = serial.Serial(
            port=port,
            baudrate=UART_BAUDRATE,
            parity='N', stopbits=1, bytesize=8, timeout=0.5
        )
        print("Serial port opened.")
    except:
        print("Failed to open serial port.")
        return False

    try:
        with open(file_path, "rb") as f:
            image_data = f.read()
        print(f"Firmware loaded: {len(image_data)} bytes.")
    except:
        print("Failed to read firmware.")
        ser.close()
        return False

    while True:
        # -------------------- 1. 发送 0x05 握手 --------------------
        print("\nSending 0x05 handshake.")
        ser.write(bytes([0x5C, 0x05]))
        ok, stat = read_ack(ser, 0x05, 1000)
        if not ok:
            print("0x05 handshake failed.")
            continue

        # ===================== CONFIG_TEST_DL_SRAM=1 流程 =====================
        if stat == 0:
            # -------------------- 2. 发送 0x06 + bin_array[64] (你漏掉的关键) --------------------
            print("Sending 0x06 + bin_array[64].")
            frame = bytearray(66)
            frame[0] = 0x5C
            frame[1] = 0x06
            frame[2:66] = bytes(bin_array[:64])  # 64字节
            ser.write(frame)
            ok, _ = read_ack(ser, 0x06, 1000)
            if not ok:
                print("bin_array send failed.")
                continue

            # -------------------- 3. 发送初始化 0x07 (固定帧) --------------------
            print("Sending initialization 0x07.")
            time.sleep(0.0001)
            init07 = bytes([
                0x5C, 0x07,
                0x00,0x00,0x00,0x00,
                0x00,0x00,0x00,
                0x04,0x00,0x00,0x00,0x00
            ])
            ser.write(init07)
            ok, _ = read_ack(ser, 0x07, 1000)
            if not ok:
                print("Initialization 0x07 failed.")
                continue
            time.sleep(0.0001)
        else:
            continue

        # -------------------- 4. 发送真正的镜像头 0x06 --------------------
        print("Sending firmware header.")
        header = generate_image_header(image_data)
        ser.write(header[:66])
        ok, stat = read_ack(ser, 0x06, 5000)
        if not ok or stat != 1:
            print("Firmware header acknowledgment failed.")
            continue

        # -------------------- 5. 发送固件数据 0x07 + 固件 --------------------
        print("Sending firmware data.")
        offset = 0
        data_len = len(image_data) - 64
        cmd = bytearray(10)
        cmd[0:2] = [0x5C, 0x07]
        # cmd[2:6] = offset.to_bytes(4, 'little')
        cmd[2] = (offset >> 24) & 0xFF  # 最高位
        cmd[3] = (offset >> 16) & 0xFF
        cmd[4] = (offset >> 8)  & 0xFF
        cmd[5] = (offset >> 0)  & 0xFF  # 最低位
        # cmd[6:10] = data_len.to_bytes(4, 'little')
        # 正确：手动按 高字节 → 低字节 放入 cmd[6] ~ cmd[9]
        cmd[6] = (data_len >> 24) & 0xFF  # 最高位
        cmd[7] = (data_len >> 16) & 0xFF
        cmd[8] = (data_len >> 8)  & 0xFF
        cmd[9] = (data_len >> 0)  & 0xFF  # 最低位
        ser.write(cmd)
        ser.write(image_data[64:])

        ok, _ = read_ack(ser, 0x07, 5000)
        if not ok:
            continue

        # -------------------- 6. 发送 0x08 启动 --------------------
        print("Sending 0x08 verification.")
        ser.write(bytes([0x5C, 0x08]))
        ok, stat = read_ack(ser, 0x08, 30000)
        if ok and stat == 3:
            print("Sending 0x09 start command.")
            ser.write(bytes([0x5C, 0x09]))
            print("\nDownload completed. Starting application.")
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            time.sleep(0.1)
            break

    ser.close()
    return True

# 示例用法
# if __name__ == "__main__":
#     port_list = list_serial_ports()
#     if not port_list:
#         print("未检测到可用串口，推出程序")
#         sys.exit(0)
#     try:
#         select_idx = int(input("请输入串口序号：")) - 1
#         download_serial = port_list[select_idx].device
#     except:
#         print("串口选择错误，退出")
#         sys.exit(0)

#     auto_imgwriter_path = Path("./imgwriter.bin")
#     if auto_imgwriter_path.exists():
#         firmware_path = str(auto_imgwriter_path.absolute())
#         print(f"✅ 自动检测到 imgwriter.bin：{firmware_path}")
#     else:
#         firmware_path = input("请输入imgwriter文件完整路径：").strip()
#         if not Path(firmware_path).exists():
#             print("错误：文件不存在，请检查路径！")
#             sys.exit(0)

#     # app_bin_path = input("请输入要下载的文件完整路径：").strip()
#     # if not Path(app_bin_path).exists():
#     #     print("错误：文件不存在，请检查路径！")
#     #     sys.exit(0)
#     imgwrite_download(download_serial, firmware_path)
#     # 文件烧录串口
#     download_baudrate = 921600
#     while True:
#         print("\n================================================")
#         print("        等待输入 APP 固件路径（输入 exit 退出）")
#         print("================================================\n")

#         # 输入文件路径
#         app_bin_path = input("请输入要烧录的APP文件完整路径：").strip()

#         # 退出指令
#         if app_bin_path.lower() == "exit":
#             print("\n程序退出...")
#             time.sleep(1)
#             break

#         # 文件检查
#         if not Path(app_bin_path).exists():
#             print("❌ 文件不存在，请重新输入！")
#             continue

#         # 开始烧录 APP
#         try:
#             print(f"\n$$$$$$$$$ 开始烧录 APP 固件 $$$$$$$$$")
#             app_burn_execution(download_serial, download_baudrate, app_bin_path)
#             print(f"\n🎉🎉🎉 APP 烧录完成！🎉🎉🎉")
#         except Exception as e:
#             print(f"\n❌ 烧录失败：{str(e)}")

#         # 完成后提示
#         print("\n✅ 准备就绪，可继续烧录下一个文件...")

if __name__ == "__main__":
    import signal

    parser = argparse.ArgumentParser(
        description="JiuChen ISP downloader. Omit firmware to use the default project image."
    )
    parser.add_argument("port", help="Serial port, for example COM3 on Windows")
    parser.add_argument(
        "firmware",
        nargs="?",
        help="Optional firmware path. Defaults to *0x18000000_V1.0.0.bin in the project root.",
    )
    args = parser.parse_args()

    # 定义 Ctrl+C 信号处理函数
    def handle_exit(signum, frame):
        print("\n\nCtrl+C detected. Exiting safely.")
        time.sleep(0.5)
        sys.exit(0)

    # 注册信号
    signal.signal(signal.SIGINT, handle_exit)

    try:
        download_serial = args.port
        print(f"Using serial port: {download_serial}")

        auto_imgwriter_path = default_imgwriter_path()
        if auto_imgwriter_path.exists():
            firmware_path = str(auto_imgwriter_path.absolute())
            print(f"Using imgwriter.bin: {firmware_path}")
        else:
            print("错误：项目根目录中未找到 imgwriter.bin")
            sys.exit(1)

        if not imgwrite_download(download_serial, firmware_path):
            raise RuntimeError("imgwriter download failed")

        if args.firmware:
            app_bin_path = Path(args.firmware)
            if not app_bin_path.is_file():
                raise FileNotFoundError(f"APP firmware was not found: {app_bin_path}")
            print(f"Using APP firmware: {app_bin_path}")
        else:
            app_bin_path = default_firmware_path()
            print(f"Using APP firmware: {app_bin_path}")
        print("\n$$$$$$$$$ 开始烧录 APP 固件 $$$$$$$$$")
        app_burn_execution(download_serial, 921600, str(app_bin_path))
        print("\nAPP download completed.")

    except KeyboardInterrupt:
        print("\n\nExited safely.")
        sys.exit(0)
    except Exception as e:
        print(f"Download failed: {e}", file=sys.stderr)
        sys.exit(1)
