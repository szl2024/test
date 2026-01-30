import os
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, messagebox


def get_bundle_dir():
    if getattr(sys, "frozen", False):
        meipass = getattr(sys, "_MEIPASS", None)
        if meipass:
            return meipass
    return os.path.dirname(os.path.abspath(__file__))


def get_app_dir():
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))


BUNDLE_DIR = get_bundle_dir()
APP_DIR = get_app_dir()
CLI_EXE = os.path.join(BUNDLE_DIR, "fcu_cli.exe")

TITLE = "FCU Tool GUI"
WINDOW_SIZE = "760x360"
STATUS_IDLE = "대기"
STATUS_RUNNING = "실행 중... 시간이 걸릴 수 있습니다"
STATUS_DONE = "완료"
STATUS_FAILED = "실패"


def validate_dir(path):
    if not path.strip():
        return "경로가 비어 있습니다"
    if not os.path.exists(path):
        return "경로가 존재하지 않습니다"
    if not os.path.isdir(path):
        return "폴더 경로가 아닙니다"
    return None


def build_cli():
    if getattr(sys, "frozen", False):
        raise RuntimeError("패키징 버전에서는 fcu_cli.exe를 자동 빌드할 수 없습니다")
    cmd = ["go", "build", "-o", CLI_EXE, "./cmd/fcu_cli"]
    result = subprocess.run(
        cmd,
        cwd=APP_DIR,
        capture_output=True,
        text=True,
        encoding="utf-8",
    )
    if result.returncode != 0:
        err = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError("go build 실패: " + (err or "알 수 없는 오류"))


def get_creation_flags():
    if os.name == "nt" and hasattr(subprocess, "CREATE_NO_WINDOW"):
        return subprocess.CREATE_NO_WINDOW
    return 0


def run_pipeline(connector_dir, model_dir, on_progress):
    if not os.path.exists(CLI_EXE):
        build_cli()

    cmd = [
        CLI_EXE,
        "--quiet",
        "--connector-dir",
        connector_dir,
        "--model-dir",
        model_dir,
    ]
    output_path = ""
    lines = []

    process = subprocess.Popen(
        cmd,
        cwd=APP_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        creationflags=get_creation_flags(),
        bufsize=1,
    )

    if process.stdout is None:
        raise RuntimeError("프로세스 출력 스트림을 열 수 없습니다")

    for raw_line in process.stdout:
        line = (raw_line or "").strip()
        if not line:
            continue
        lines.append(line)
        if line.startswith("PROGRESS:"):
            try:
                percent = int(line.split(":", 1)[1].strip())
            except ValueError:
                percent = None
            if percent is not None:
                on_progress(percent)
            continue
        output_path = line

    process.wait()
    if process.returncode != 0:
        err = "\n".join(lines[-5:]) if lines else "실행 실패"
        raise RuntimeError(err)

    if not output_path:
        raise RuntimeError("출력 경로를 가져오지 못했습니다")

    return output_path


def main():
    root = tk.Tk()
    root.title(TITLE)
    root.geometry(WINDOW_SIZE)
    root.resizable(False, False)

    connector_var = tk.StringVar()
    model_var = tk.StringVar()
    output_var = tk.StringVar()
    status_var = tk.StringVar(value=STATUS_IDLE)

    documents_dir = os.path.join(os.path.expanduser("~"), "Documents")

    def show_error(title, message):
        messagebox.showerror(title, message)

    def browse_connector():
        path = filedialog.askdirectory(
            title="입력 폴더 선택",
            initialdir=documents_dir,
        )
        if path:
            connector_var.set(path)

    def browse_model():
        path = filedialog.askdirectory(
            title="모델 폴더 선택",
            initialdir=documents_dir,
        )
        if path:
            model_var.set(path)

    def set_busy(is_busy):
        state = "disabled" if is_busy else "normal"
        connector_entry.config(state=state)
        model_entry.config(state=state)
        browse_connector_btn.config(state=state)
        browse_model_btn.config(state=state)
        run_btn.config(state=state)

    def on_run():
        connector_dir = connector_var.get().strip()
        model_dir = model_var.get().strip()

        err = validate_dir(connector_dir)
        if err:
            show_error("입력 오류", "입력 폴더: " + err)
            return
        err = validate_dir(model_dir)
        if err:
            show_error("입력 오류", "모델 폴더: " + err)
            return

        csv_path = os.path.join(connector_dir, "asw.csv")
        if not os.path.exists(csv_path):
            show_error("입력 오류", "asw.csv를 찾을 수 없습니다: " + csv_path)
            return

        output_var.set("")
        status_var.set(STATUS_RUNNING)
        set_busy(True)

        def report_progress(percent):
            status_var.set(f"{STATUS_RUNNING} ({percent}%)")

        def worker():
            try:
                output_path = run_pipeline(
                    connector_dir,
                    model_dir,
                    lambda p: root.after(0, lambda: report_progress(p)),
                )
                root.after(0, lambda: output_var.set(output_path))
                root.after(0, lambda: status_var.set(STATUS_DONE))
            except Exception as exc:
                root.after(0, lambda: show_error("실행 실패", str(exc)))
                root.after(0, lambda: status_var.set(STATUS_FAILED))
            finally:
                root.after(0, lambda: set_busy(False))

        threading.Thread(target=worker, daemon=True).start()

    frame = tk.Frame(root, padx=16, pady=16)
    frame.pack(fill="both", expand=True)

    tk.Label(frame, text="입력 폴더").grid(row=0, column=0, sticky="w")
    connector_entry = tk.Entry(frame, textvariable=connector_var, width=72)
    connector_entry.grid(row=1, column=0, sticky="we", pady=(4, 8))
    browse_connector_btn = tk.Button(frame, text="찾아보기", command=browse_connector)
    browse_connector_btn.grid(row=1, column=1, padx=(8, 0), pady=(4, 8))

    tk.Label(frame, text="모델 폴더").grid(row=2, column=0, sticky="w")
    model_entry = tk.Entry(frame, textvariable=model_var, width=72)
    model_entry.grid(row=3, column=0, sticky="we", pady=(4, 8))
    browse_model_btn = tk.Button(frame, text="찾아보기", command=browse_model)
    browse_model_btn.grid(row=3, column=1, padx=(8, 0), pady=(4, 8))

    tk.Label(frame, text="출력 파일").grid(row=4, column=0, sticky="w")
    output_entry = tk.Entry(frame, textvariable=output_var, width=72, state="disabled")
    output_entry.grid(row=5, column=0, sticky="we", pady=(4, 8))

    run_btn = tk.Button(frame, text="실행", command=on_run)
    run_btn.grid(row=6, column=0, sticky="w", pady=(6, 0))
    status_label = tk.Label(frame, textvariable=status_var)
    status_label.grid(row=6, column=0, sticky="e", pady=(6, 0))

    frame.columnconfigure(0, weight=1)

    root.mainloop()


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror("시작 실패", str(exc))
