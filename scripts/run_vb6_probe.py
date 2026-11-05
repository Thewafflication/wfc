"""Runs a Visual Basic 6 project in the VB6 IDE and waits for its output file.

The probes of evidence/reference write a text file next to the project. This
script opens the project in Visual Basic 6, presses Start With Full Compile
(Ctrl+F5), waits until the program has ended and the file is complete, closes
the IDE without saving, and leaves the file where the project wrote it. It uses
only the Python standard library (ctypes) and the Windows API.

The IDE needs the desktop: it must be unlocked, and the script takes the
foreground for the minute or so that a probe runs. Do not type meanwhile.

Usage:
    python scripts/run_vb6_probe.py <project.vbp> <output-file> [--timeout 300]
                                    [--vb6 <path to VB6.EXE>]

Example (generate the project first with scripts/New-SelectCaseProbe.ps1):
    python scripts/run_vb6_probe.py
        evidence/reference/select-case-probe/SelectCaseProbe.vbp
        evidence/reference/select-case-probe/select-case-probe.txt
"""

import argparse
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

DEFAULT_VB6 = r"C:\Program Files (x86)\Microsoft Visual Studio\VB98\VB6.EXE"

WM_CLOSE = 0x0010
BM_CLICK = 0x00F5
KEYEVENTF_KEYUP = 0x0002
VK_CONTROL = 0x11
VK_F5 = 0x74
VK_MENU = 0x12
SW_RESTORE = 9

EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND,
                                     wintypes.LPARAM)


def window_text(hwnd):
    length = user32.GetWindowTextLengthW(hwnd)
    buffer = ctypes.create_unicode_buffer(length + 1)
    user32.GetWindowTextW(hwnd, buffer, length + 1)
    return buffer.value


def class_name(hwnd):
    buffer = ctypes.create_unicode_buffer(256)
    user32.GetClassNameW(hwnd, buffer, 256)
    return buffer.value


def windows_of(pid):
    """The top-level windows of a process: (handle, title, class)."""
    found = []

    def visit(hwnd, _):
        owner = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value == pid and user32.IsWindowVisible(hwnd):
            found.append((hwnd, window_text(hwnd), class_name(hwnd)))
        return True

    user32.EnumWindows(EnumWindowsProc(visit), 0)
    return found


def children_of(hwnd):
    found = []

    def visit(child, _):
        found.append((child, window_text(child), class_name(child)))
        return True

    user32.EnumChildWindows(hwnd, EnumWindowsProc(visit), 0)
    return found


def main_window(pid, project_name):
    """The IDE window: `<project> - Microsoft Visual Basic [mode]`."""
    for hwnd, title, _ in windows_of(pid):
        if (project_name.lower() in title.lower()
                and "Microsoft Visual Basic" in title):
            return hwnd, title
    return None, ""


def to_foreground(hwnd):
    """Brings a window to the front (Windows refuses unless the caller holds
    the foreground: a tap of Alt and a thread attach get round that)."""
    user32.ShowWindow(hwnd, SW_RESTORE)
    user32.keybd_event(VK_MENU, 0, 0, 0)
    user32.keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0)
    foreground = user32.GetForegroundWindow()
    this_thread = kernel32.GetCurrentThreadId()
    other_thread = user32.GetWindowThreadProcessId(foreground, None)
    user32.AttachThreadInput(this_thread, other_thread, True)
    user32.SetForegroundWindow(hwnd)
    user32.BringWindowToTop(hwnd)
    user32.AttachThreadInput(this_thread, other_thread, False)


def press_ctrl_f5():
    user32.keybd_event(VK_CONTROL, 0, 0, 0)
    user32.keybd_event(VK_F5, 0, 0, 0)
    user32.keybd_event(VK_F5, 0, KEYEVENTF_KEYUP, 0)
    user32.keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0)


def answer_dialogs(pid, main):
    """Dismisses a dialog of the IDE: `No` to a question about saving, the
    default button otherwise. Returns what it saw."""
    seen = []
    for hwnd, title, cls in windows_of(pid):
        if hwnd == main or cls != "#32770":
            continue
        buttons = [(c, t) for c, t, k in children_of(hwnd) if k == "Button"]
        text = " / ".join(t for _, t, k in children_of(hwnd) if k == "Static")
        seen.append(f"{title}: {text}")
        chosen = None
        for button, label in buttons:
            if label.replace("&", "").lower() in ("no", "don't save"):
                chosen = button
        if chosen is None and buttons:
            chosen = buttons[0][0]
        if chosen is not None:
            user32.SendMessageW(chosen, BM_CLICK, 0, 0)
    return seen


def file_state(path):
    try:
        stat = os.stat(path)
        return stat.st_size, stat.st_mtime
    except OSError:
        return None


def run(project, output, timeout, vb6):
    project = os.path.abspath(project)
    output = os.path.abspath(output)
    if os.path.exists(output):
        os.remove(output)
    name = os.path.splitext(os.path.basename(project))[0]
    process = subprocess.Popen([vb6, project])
    deadline = time.time() + timeout
    main = None
    title = ""
    # Wait for the IDE to load the project.
    while time.time() < deadline:
        main, title = main_window(process.pid, name)
        if main and "[design]" in title:
            break
        answer_dialogs(process.pid, main)
        time.sleep(0.5)
    else:
        process.kill()
        raise SystemExit("the IDE did not open the project in time")
    time.sleep(1.0)
    to_foreground(main)
    time.sleep(0.5)
    press_ctrl_f5()
    # Wait for the program to run and end: the title says [run] and then
    # [design] again, and the output file stops growing.
    saw_run = False
    problems = []
    last = None
    stable_since = None
    while time.time() < deadline:
        main, title = main_window(process.pid, name)
        problems += answer_dialogs(process.pid, main)
        if "[run]" in title:
            saw_run = True
        state = file_state(output)
        if state != last:
            last = state
            stable_since = time.time()
        elif (state is not None and stable_since is not None
              and time.time() - stable_since > 3.0
              and "[design]" in title and (saw_run or state[0] > 0)):
            break
        time.sleep(0.5)
    else:
        process.kill()
        raise SystemExit("the program did not finish in time; seen: "
                         + "; ".join(problems))
    # Close the IDE without saving.
    if main:
        user32.PostMessageW(main, WM_CLOSE, 0, 0)
    for _ in range(40):
        if process.poll() is not None:
            break
        main, _ = main_window(process.pid, name)
        answer_dialogs(process.pid, main)
        time.sleep(0.25)
    if process.poll() is None:
        process.kill()
    if not os.path.exists(output):
        raise SystemExit("the project wrote no output file; seen: "
                         + "; ".join(problems))
    size = os.path.getsize(output)
    print(f"{output}: {size} bytes")
    for line in problems:
        print("dialog:", line)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("project", help="the .vbp to run")
    parser.add_argument("output", help="the file the project writes")
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--vb6", default=DEFAULT_VB6)
    arguments = parser.parse_args()
    if sys.platform != "win32":
        raise SystemExit("Windows only")
    run(arguments.project, arguments.output, arguments.timeout, arguments.vb6)


if __name__ == "__main__":
    main()
