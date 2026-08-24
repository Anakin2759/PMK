import argparse
import os
import platform
import signal
import shutil
import subprocess
import sys
import time
from pathlib import Path


IS_WINDOWS = platform.system() == "Windows"

WINDOWS_CRASH_CODES = {
    0xC0000005: "访问冲突（读写了无效内存）",
    0xC000001D: "非法指令",
    0xC0000025: "不可继续执行的异常",
    0xC000008C: "数组越界",
    0xC000008D: "浮点反规格化操作数",
    0xC000008E: "浮点除零",
    0xC000008F: "浮点不精确结果",
    0xC0000090: "浮点无效操作",
    0xC0000091: "浮点溢出",
    0xC0000092: "浮点栈检查失败",
    0xC0000093: "浮点下溢",
    0xC0000094: "整数除零",
    0xC0000095: "整数溢出",
    0xC0000096: "特权指令",
    0xC00000FD: "栈溢出",
    0xC0000135: "找不到依赖 DLL",
    0xC0000139: "找不到 DLL 入口点",
    0xC0000142: "DLL 初始化失败",
    0xC0000374: "堆损坏",
    0xC0000409: "快速失败或栈缓冲区越界",
    0xE06D7363: "未处理的 Microsoft C++ 异常",
}


def unsigned_exit_code(return_code: int) -> int:
    """将可能为有符号形式的 Windows 退出码规范化为 32 位无符号值。"""
    return return_code & 0xFFFFFFFF


def classify_exit(return_code: int) -> tuple[bool, str]:
    if IS_WINDOWS:
        code = unsigned_exit_code(return_code)
        if code in WINDOWS_CRASH_CODES:
            return True, WINDOWS_CRASH_CODES[code]
        if 0xC0000000 <= code <= 0xCFFFFFFF or 0xE0000000 <= code <= 0xEFFFFFFF:
            return True, "未处理的 Windows 异常"
        if code == 0:
            return False, "正常退出"
        return False, "程序返回非零退出码"

    if return_code < 0:
        signal_number = -return_code
        try:
            signal_name = signal.Signals(signal_number).name
        except ValueError:
            signal_name = f"信号 {signal_number}"
        return True, f"被 {signal_name} 终止"
    if return_code == 0:
        return False, "正常退出"
    return False, "程序返回非零退出码"


def print_output(title: str, content: str) -> None:
    if not content:
        return
    print(f"\n{title}:")
    print(content.rstrip())


def stop_process(process: subprocess.Popen[str]) -> None:
    """结束观察期后仍在运行的进程，避免诊断脚本遗留后台程序。"""
    process.terminate()
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=3)


def stop_process_tree(process: subprocess.Popen[str]) -> None:
    """终止调试器及其被调试子进程。"""
    if IS_WINDOWS:
        subprocess.run(
            ["taskkill", "/PID", str(process.pid), "/T", "/F"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        process.wait(timeout=5)
        return
    stop_process(process)


def find_lldb() -> str | None:
    candidates = [shutil.which("lldb")]
    if IS_WINDOWS:
        candidates.append(r"D:\LLVM\bin\lldb.exe")
    return next((candidate for candidate in candidates if candidate and Path(candidate).is_file()), None)


def monitor_with_lldb(exe_path: Path, duration: float, arguments: list[str], lldb: str) -> int:
    """由 LLDB 捕获未处理异常，以及进程主动退出前一瞬间的线程堆栈。"""
    command = [
        lldb,
        "--batch",
        "--no-lldbinit",
    ]
    if IS_WINDOWS:
        # MSVC CRT 的 abort() 默认以状态 3 终止，并可能直接调用 TerminateProcess，
        # 不一定经过 ExitProcess。按从高层 C++ 终止到 Win32 退出 API 的顺序设断点，
        # 让 LLDB 在进程真正消失前保留可回溯现场。
        for symbol in (
            "__std_terminate",
            "terminate",
            "abort",
            "_invoke_watson",
            "_exit",
            "TerminateProcess",
            "ExitProcess",
            "RtlExitUserProcess",
        ):
            command.extend(["-o", f"breakpoint set --name {symbol}"])
    command.extend([
        "-o", "run",
        "-o", "register read",
        "-o", "thread backtrace all",
        "-k", "register read",
        "-k", "thread backtrace all",
        "--",
        str(exe_path),
        *arguments,
    ])

    print(f"调试器: {lldb}")
    print("堆栈捕获: 已启用（未处理异常 + 进程退出入口）")
    started_at = time.monotonic()
    try:
        process = subprocess.Popen(
            command,
            cwd=exe_path.parent,
            env=os.environ.copy(),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if IS_WINDOWS else 0,
        )
    except OSError as error:
        print(f"❌ 无法启动 LLDB: {error}")
        return 2

    try:
        output, _ = process.communicate(timeout=duration)
    except subprocess.TimeoutExpired:
        elapsed = time.monotonic() - started_at
        stop_process_tree(process)
        output, _ = process.communicate()
        print_output("LLDB 输出", output)
        print(f"\n✅ 观察 {elapsed:.2f} 秒后未捕获崩溃或退出，进程仍在运行。")
        print("说明: 调试器及被测进程已由检测脚本终止。")
        return 0

    elapsed = time.monotonic() - started_at
    print_output("LLDB 捕获结果", output)
    lower_output = output.lower()
    captured_stack = "frame #0:" in lower_output
    stop_lines = [line.strip() for line in lower_output.splitlines() if "stop reason =" in line]
    stop_reason = "\n".join(stop_lines)
    crashed = "exception" in stop_reason or "signal" in stop_reason
    stopped_at_termination = any(
        symbol in stop_reason
        for symbol in (
            "__std_terminate",
            "terminate",
            "abort",
            "_invoke_watson",
            "_exit",
            "terminateprocess",
            "exitprocess",
            "rtlexituserprocess",
        )
    )

    if crashed:
        print(f"\n💥 LLDB 在运行时异常处停止，已捕获堆栈（启动后 {elapsed:.2f} 秒）。")
        return 1
    if stopped_at_termination and captured_stack:
        print(f"\n⚠️ LLDB 在异常终止/退出路径停止，以上是进程消失前的堆栈（启动后 {elapsed:.2f} 秒）。")
        print("提示: abort() 在 Windows CRT 中通常产生退出状态 3；Win64 首个整数参数通常位于 rcx。")
        return 3
    if captured_stack:
        print(f"\n⚠️ LLDB 已停止并捕获堆栈（启动后 {elapsed:.2f} 秒），请查看 stop reason。")
        return 3

    print(f"\n⚠️ 调试会话已结束，但没有可用堆栈（LLDB 退出码 {process.returncode}）。")
    return 3


def monitor_crash(
    exe_path: Path,
    duration: float,
    arguments: list[str],
    use_debugger: bool,
) -> int:
    print("=" * 64)
    print("运行时崩溃检测")
    print("=" * 64)

    if not exe_path.is_file():
        print(f"❌ 可执行文件不存在: {exe_path}")
        return 2

    work_dir = exe_path.parent
    command = [str(exe_path), *arguments]
    creation_flags = subprocess.CREATE_NEW_PROCESS_GROUP if IS_WINDOWS else 0

    print(f"程序: {exe_path}")
    print(f"大小: {exe_path.stat().st_size / 1024:.2f} KB")
    print(f"平台: {platform.platform()}")
    print(f"工作目录: {work_dir}")
    print(f"观察时间: {duration:.1f} 秒")
    if arguments:
        print(f"程序参数: {arguments}")

    if use_debugger:
        lldb = find_lldb()
        if lldb:
            return monitor_with_lldb(exe_path, duration, arguments, lldb)
        print("⚠️ 未找到 LLDB，回退为仅检测退出码（无法捕获崩溃瞬间堆栈）。")

    started_at = time.monotonic()
    try:
        process = subprocess.Popen(
            command,
            cwd=work_dir,
            env=os.environ.copy(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creation_flags,
        )
    except OSError as error:
        winerror = getattr(error, "winerror", None)
        suffix = f" (WinError {winerror})" if winerror is not None else ""
        print(f"❌ 无法启动程序{suffix}: {error}")
        return 2

    print(f"进程 PID: {process.pid}")

    try:
        stdout, stderr = process.communicate(timeout=duration)
    except subprocess.TimeoutExpired:
        elapsed = time.monotonic() - started_at
        print(f"\n✅ 观察 {elapsed:.2f} 秒后进程仍在运行，未检测到早期崩溃。")
        stop_process(process)
        stdout, stderr = process.communicate()
        print_output("标准输出", stdout)
        print_output("标准错误", stderr)
        print("说明: 进程由检测脚本在观察期结束后主动终止。")
        return 0

    elapsed = time.monotonic() - started_at
    return_code = process.returncode
    crashed, reason = classify_exit(return_code)
    print_output("标准输出", stdout)
    print_output("标准错误", stderr)

    if IS_WINDOWS:
        code_text = f"0x{unsigned_exit_code(return_code):08X}"
    else:
        code_text = str(return_code)

    if crashed:
        print(f"\n💥 检测到运行时崩溃: {reason}")
        print(f"退出码: {return_code} ({code_text})")
        print(f"崩溃时间: 启动后 {elapsed:.2f} 秒")
        return 1

    icon = "✅" if return_code == 0 else "⚠️"
    print(f"\n{icon} {reason}")
    print(f"退出码: {return_code} ({code_text})")
    print(f"运行时间: {elapsed:.2f} 秒")
    return 0 if return_code == 0 else 3


def parse_args() -> argparse.Namespace:
    default_exe = Path(__file__).parent / "build" / "example" / "ui_demo" / "example_ui_demo.exe"
    parser = argparse.ArgumentParser(description="启动可执行文件并检测早期运行时崩溃。")
    parser.add_argument("exe", nargs="?", type=Path, default=default_exe, help="待检测的可执行文件")
    parser.add_argument("--duration", type=float, default=10.0, help="观察秒数，默认 10 秒")
    parser.add_argument("--no-debugger", action="store_true", help="不使用 LLDB，仅根据退出码检测崩溃")
    parser.add_argument("program_args", nargs=argparse.REMAINDER, help="传给被测程序的参数（放在 -- 之后）")
    args = parser.parse_args()
    if args.duration <= 0:
        parser.error("--duration 必须大于 0")
    if args.program_args[:1] == ["--"]:
        args.program_args = args.program_args[1:]
    return args


if __name__ == "__main__":
    parsed = parse_args()
    sys.exit(
        monitor_crash(
            parsed.exe.resolve(),
            parsed.duration,
            parsed.program_args,
            not parsed.no_debugger,
        )
    )