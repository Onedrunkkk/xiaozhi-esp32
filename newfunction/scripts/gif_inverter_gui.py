# -*- coding: utf-8 -*-
"""
🎬 视频工具箱 v4.1 — 带日志分析：精准定位 GIF 变慢问题 + GIF 处理修复
"""

import os
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from moviepy.editor import VideoFileClip, concatenate_videoclips, ImageClip
from PIL import Image, ImageOps, ImageSequence
import imageio
import numpy as np
import logging
import time
from datetime import datetime

# ==========================
# 日志系统配置
# ==========================

LOG_FILE = "video_toolbox.log"

# 清理旧日志
if os.path.exists(LOG_FILE):
    os.remove(LOG_FILE)

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE, encoding='utf-8'),
        logging.StreamHandler()
    ]
)

logger = logging.getLogger(__name__)

# 全局变量
inversion_files = []
concatenation_files = []
log_text_widget = None  # GUI 日志显示控件


# ==========================
# 工具函数
# ==========================

def log_message(msg):
    """同时输出到日志文件和 GUI 文本框"""
    logger.info(msg)
    if log_text_widget:
        log_text_widget.insert(tk.END, msg + "\n")
        log_text_widget.see(tk.END)


def get_output_path(original_path, output_dir, suffix="_Inversion"):
    base_name, ext = os.path.splitext(os.path.basename(original_path))
    return os.path.join(output_dir, f"{base_name}{suffix}{ext}")


# --------------------------
# 文件选择
# --------------------------

def select_files_for_inversion():
    global inversion_files
    files = filedialog.askopenfilenames(
        title="选择用于颜色反转的文件",
        filetypes=[
            ("视频与图片", "*.mp4 *.avi *.mov *.webm *.gif *.jpg *.jpeg *.png *.bmp"),
            ("视频", "*.mp4 *.avi *.mov *.webm *.gif"),
            ("图片", "*.jpg *.jpeg *.png *.bmp"),
            ("全部文件", "*.*")
        ]
    )
    if files:
        new_files = [os.path.abspath(f) for f in files]
        unique_files = [f for f in new_files if f not in inversion_files]
        inversion_files.extend(unique_files)
        update_inversion_listbox()
        log_message(f"✅ 添加 {len(unique_files)} 个文件用于颜色反转")


def select_files_for_concatenation():
    global concatenation_files
    files = filedialog.askopenfilenames(
        title="选择用于拼接的视频文件",
        filetypes=[("视频文件", "*.mp4 *.avi *.mov *.webm"), ("全部文件", "*.*")]
    )
    if files:
        new_files = [os.path.abspath(f) for f in files]
        unique_files = [f for f in new_files if f not in concatenation_files]
        concatenation_files.extend(unique_files)
        update_concatenation_listbox()
        log_message(f"✅ 添加 {len(unique_files)} 个文件用于拼接")


# --------------------------
# 列表更新 + 删除
# --------------------------

def update_inversion_listbox():
    listbox_inversion.delete(0, tk.END)
    for f in inversion_files:
        listbox_inversion.insert(tk.END, os.path.basename(f))


def update_concatenation_listbox():
    listbox_concat.delete(0, tk.END)
    for f in concatenation_files:
        listbox_concat.insert(tk.END, os.path.basename(f))


def remove_inversion_file():
    selected = listbox_inversion.curselection()
    if not selected:
        return
    index = selected[0]
    fname = inversion_files[index]
    del inversion_files[index]
    update_inversion_listbox()
    log_message(f"🗑️ 删除文件: {os.path.basename(fname)}")


def remove_concat_file():
    selected = listbox_concat.curselection()
    if not selected:
        return
    index = selected[0]
    fname = concatenation_files[index]
    del concatenation_files[index]
    update_concatenation_listbox()
    log_message(f"🗑️ 删除文件: {os.path.basename(fname)}")


# --------------------------
# 拼接顺序操作
# --------------------------

def move_up():
    selected = listbox_concat.curselection()
    if not selected:
        return
    index = selected[0]
    if index == 0:
        return
    concatenation_files[index], concatenation_files[index - 1] = \
        concatenation_files[index - 1], concatenation_files[index]
    update_concatenation_listbox()
    listbox_concat.selection_set(index - 1)
    log_message("↑ 拼接顺序上移")


def move_down():
    selected = listbox_concat.curselection()
    if not selected:
        return
    index = selected[0]
    if index >= len(concatenation_files) - 1:
        return
    concatenation_files[index], concatenation_files[index + 1] = \
        concatenation_files[index + 1], concatenation_files[index]
    update_concatenation_listbox()
    listbox_concat.selection_set(index + 1)
    log_message("↓ 拼接顺序下移")


# --------------------------
# 处理函数（带日志）
# --------------------------

def process_gif_to_inverted_file(input_path, output_path):
    try:
        log_message(f"🖼️ 处理 GIF: {input_path}")
        img = Image.open(input_path)
        log_message(f"   📊 原始信息: mode={img.mode}, size={img.size}, format={img.format}")

        # 提取原始 duration
        durations_ms = []
        for i, frame in enumerate(ImageSequence.Iterator(img)):
            dur = frame.info.get('duration', 40)  # 默认 40ms (25fps)
            durations_ms.append(dur)
            if i < 5:
                log_message(f"   🕒 第{i+1}帧 duration: {dur}ms")

        avg_duration_ms = sum(durations_ms) / len(durations_ms)
        fps_original = 1000 / avg_duration_ms if avg_duration_ms > 0 else 30
        log_message(f"   📈 平均帧间隔: {avg_duration_ms:.2f}ms → 帧率: {fps_original:.2f}fps")

        # 反色处理并保存每帧
        inverted_frames = []
        for i, frame in enumerate(ImageSequence.Iterator(img)):
            # 转为灰度并反色
            frame_gray = frame.convert('L')
            inverted_frame = ImageOps.invert(frame_gray)
            inverted_frame = inverted_frame.convert('P')  # 转回调色板模式

            # 保留原始 duration
            inverted_frame.info['duration'] = durations_ms[i]
            inverted_frame.info['loop'] = frame.info.get('loop', 0)

            inverted_frames.append(inverted_frame)

        # 使用 Pillow 保存，精确控制每帧 delay
        if inverted_frames:
            # 保存第一帧
            inverted_frames[0].save(
                output_path,
                save_all=True,
                append_images=inverted_frames[1:],
                duration=durations_ms,
                loop=img.info.get('loop', 0),
                optimize=True,
                disposal=2  # 透明背景处理
            )

        # 验证输出
        output_img = Image.open(output_path)
        output_durations = []
        for frame in ImageSequence.Iterator(output_img):
            output_durations.append(frame.info.get('duration', 40))
        avg_out_duration = sum(output_durations) / len(output_durations)
        fps_output = 1000 / avg_out_duration if avg_out_duration > 0 else 30
        log_message(f"   ✅ 输出验证: 平均帧间隔 {avg_out_duration:.2f}ms → 帧率 {fps_output:.2f}fps")

        if abs(fps_original - fps_output) > 1:
            log_message("   ⚠️ 警告: 输入输出帧率差异较大！")
        else:
            log_message("   ✅ 帧率保持良好，GIF 速度正常")

        log_message(f"   💾 输出: {output_path}")
        return True, None

    except Exception as e:
        error_msg = f"❌ GIF 处理失败: {str(e)}"
        log_message(error_msg)
        return False, str(e)


def process_video_to_inverted_file(input_path, output_path, max_width=800):
    try:
        ext = os.path.splitext(input_path)[-1].lower()
        start_time = time.time()

        if ext == ".gif":
            return process_gif_to_inverted_file(input_path, output_path)
        else:
            log_message(f"🎥 处理视频: {input_path}")
            reader = imageio.get_reader(input_path)
            meta = reader.get_meta_data()
            fps = meta.get('fps', 30)
            duration = meta.get('duration', 0)
            log_message(f"   📊 原始元数据: fps={fps}, duration={duration:.2f}s, size={meta.get('size')}")

            writer = imageio.get_writer(output_path, fps=fps, codec='libx264', quality=8)

            frame_count = 0
            for frame in reader:
                if frame_count == 0:
                    h, w = frame.shape[:2]
                    if w > max_width:
                        new_h = int(h * max_width / w)
                        new_w = max_width
                    else:
                        new_w, new_h = w, h
                    log_message(f"   🖼️ 缩放: {w}x{h} → {new_w}x{new_h}")

                # 反色
                if len(frame.shape) == 3:
                    gray = np.dot(frame[...,:3], [0.2989, 0.5870, 0.1140]).astype(np.uint8)
                else:
                    gray = frame
                inverted = 255 - gray
                rgb_inverted = np.stack([inverted]*3, axis=-1)

                if w > max_width:
                    import cv2
                    rgb_inverted = cv2.resize(rgb_inverted, (new_w, new_h), interpolation=cv2.INTER_AREA)

                writer.append_data(rgb_inverted)
                frame_count += 1

            log_message(f"   ✅ 处理 {frame_count} 帧")
            writer.close()
            reader.close()

        elapsed = time.time() - start_time
        log_message(f"   🕒 处理耗时: {elapsed:.2f} 秒")
        log_message(f"   💾 输出: {output_path}")
        return True, None

    except Exception as e:
        error_msg = f"❌ 处理失败: {str(e)}"
        log_message(error_msg)
        return False, str(e)


def process_image_to_inverted_file(image_path, output_path):
    try:
        log_message(f"🖼️ 处理图片: {image_path}")
        img = Image.open(image_path)
        result_img = ImageOps.invert(img.convert('L')).convert('RGB')
        result_img.save(output_path.replace('.mp4', '.jpg'))
        log_message(f"   💾 保存为图片: {output_path.replace('.mp4', '.jpg')}")
        return True, None
    except Exception as e:
        log_message(f"❌ 图片处理失败: {e}")
        return False, e


def start_inversion():
    if not inversion_files:
        messagebox.showwarning("⚠️ 操作提示", "请先选择要处理的文件！")
        return

    output_dir = filedialog.askdirectory(title="选择批量输出目录")
    if not output_dir:
        return

    progress_bar.start(10)
    root.update()

    success_count = 0
    failed_list = []

    for input_path in inversion_files:
        log_message(f"🔄 开始处理: {os.path.basename(input_path)}")
        try:
            output_path = get_output_path(input_path, output_dir, "_Inversion")
            ext = os.path.splitext(input_path)[-1].lower()

            if ext in ['.mp4', '.avi', '.mov', '.webm']:
                success, error = process_video_to_inverted_file(input_path, output_path)
            elif ext == '.gif':
                success, error = process_gif_to_inverted_file(input_path, output_path)
            elif ext in ['.jpg', '.jpeg', '.png', '.bmp']:
                success, error = process_image_to_inverted_file(input_path, output_path)
            else:
                success, error = False, "不支持的格式"

            if success:
                success_count += 1
            else:
                failed_list.append(f"{os.path.basename(input_path)}: {error}")

        except Exception as e:
            failed_list.append(f"{os.path.basename(input_path)}: {e}")

    progress_bar.stop()

    msg = f"✅ 成功处理 {success_count} 个文件"
    if failed_list:
        msg += f"\n❌ 失败 {len(failed_list)} 个：\n" + "\n".join(failed_list[:5])
        if len(failed_list) > 5:
            msg += f"\n... 还有 {len(failed_list)-5} 个错误"
    else:
        msg += "\n🎉 所有文件处理成功！"

    messagebox.showinfo("批量处理完成", msg)
    log_message("🔚 批量处理完成")


def start_concatenation():
    if len(concatenation_files) < 2:
        messagebox.showwarning("⚠️ 操作提示", "请至少选择两个视频文件进行拼接！")
        return

    output_path = filedialog.asksaveasfilename(
        title="保存拼接后的视频",
        defaultextension=".mp4",
        filetypes=[("MP4 文件", "*.mp4"), ("AVI 文件", "*.avi"), ("MOV 文件", "*.mov")]
    )
    if not output_path:
        return

    try:
        fps = int(fps_concat_var.get())
    except:
        fps = 30

    clips = []
    for path in concatenation_files:
        try:
            clip = VideoFileClip(path)
            log_message(f"🎥 加载视频: {path}, duration={clip.duration:.2f}s, fps={clip.fps}")
            clips.append(clip)
        except Exception as e:
            log_message(f"❌ 加载失败: {path}, error={e}")
            messagebox.showerror("❌ 错误", f"加载失败: {path}\n{e}")
            return

    final_clip = concatenate_videoclips(clips)
    log_message(f"🔗 拼接完成，总时长: {final_clip.duration:.2f}s")

    final_clip.write_videofile(output_path, fps=fps, codec='libx264', audio=False, preset='medium')
    log_message(f"💾 保存拼接视频: {output_path}")

    for c in clips:
        c.close()
    final_clip.close()

    messagebox.showinfo("✅ 成功", f"拼接完成（按顺序）：\n{output_path}")
    log_message("🔚 视频拼接完成")


# ==========================
# 主界面
# ==========================

root = tk.Tk()
root.title("🎬 视频工具箱 — 调试版 v4.1")
root.geometry("960x800")
root.resizable(False, False)
root.configure(bg="#f0f2f5")

title_font = ("微软雅黑", 13, "bold")
label_font = ("微软雅黑", 9)
button_font = ("微软雅黑", 9)

style = ttk.Style()
style.theme_use("clam")
style.configure("TButton", font=button_font, padding=6)
style.configure("TLabel", font=label_font, background="#f0f2f5")
style.configure("TLabelframe", font=title_font, padding=10)

main_frame = ttk.Frame(root, padding="20")
main_frame.pack(fill="both", expand=True)

# === 工具 1：颜色反转 ===
frame_invert = ttk.LabelFrame(main_frame, text="🎨 颜色反转（带日志分析）", padding=15)
frame_invert.pack(fill="x", pady=(0, 15))

ttk.Button(frame_invert, text="📁 选择文件", command=select_files_for_inversion).grid(row=0, column=0, sticky="w", pady=5)

list_frame_invert = ttk.Frame(frame_invert)
list_frame_invert.grid(row=1, column=0, columnspan=3, pady=5, sticky="ew")

listbox_inversion = tk.Listbox(list_frame_invert, height=4, width=60, font=("Consolas", 9), bg="white", bd=0, highlightthickness=1, highlightbackground="#ccc")
scrollbar_invert = ttk.Scrollbar(list_frame_invert, orient="vertical", command=listbox_inversion.yview)
listbox_inversion.config(yscrollcommand=scrollbar_invert.set)
listbox_inversion.pack(side="left", fill="x", expand=True)
scrollbar_invert.pack(side="right", fill="y")

btn_frame_invert = ttk.Frame(frame_invert)
btn_frame_invert.grid(row=2, column=0, columnspan=3, pady=5, sticky="w")
ttk.Button(btn_frame_invert, text="❌ 删除选中", command=remove_inversion_file).pack(side="left", padx=2)

ttk.Button(frame_invert, text="✅ 批量开始反色", command=start_inversion, width=20).grid(row=3, column=0, columnspan=3, pady=10)

# === 工具 2：视频拼接 ===
frame_concat = ttk.LabelFrame(main_frame, text="🔗 视频拼接", padding=15)
frame_concat.pack(fill="x", pady=(0, 15))

ttk.Button(frame_concat, text="📁 选择文件", command=select_files_for_concatenation).grid(row=0, column=0, sticky="w", pady=5)

list_frame_concat = ttk.Frame(frame_concat)
list_frame_concat.grid(row=1, column=0, columnspan=3, pady=5, sticky="ew")

listbox_concat = tk.Listbox(list_frame_concat, height=4, width=60, font=("Consolas", 9), bg="white", bd=0, highlightthickness=1, highlightbackground="#ccc")
scrollbar_concat = ttk.Scrollbar(list_frame_concat, orient="vertical", command=listbox_concat.yview)
listbox_concat.config(yscrollcommand=scrollbar_concat.set)
listbox_concat.pack(side="left", fill="x", expand=True)
scrollbar_concat.pack(side="right", fill="y")

btn_frame_concat = ttk.Frame(frame_concat)
btn_frame_concat.grid(row=2, column=0, columnspan=3, pady=5, sticky="w")
ttk.Button(btn_frame_concat, text="❌ 删除选中", command=remove_concat_file).pack(side="left", padx=2)
ttk.Button(btn_frame_concat, text="↑ 上移", command=move_up).pack(side="left", padx=2)
ttk.Button(btn_frame_concat, text="↓ 下移", command=move_down).pack(side="left", padx=2)

fps_concat_var = tk.StringVar(value="30")
ttk.Entry(frame_concat, textvariable=fps_concat_var, width=5).grid(row=3, column=0, sticky="w", pady=5)
ttk.Label(frame_concat, text="FPS").grid(row=3, column=1, sticky="w", pady=5)

ttk.Button(frame_concat, text="✅ 开始拼接", command=start_concatenation, width=20).grid(row=4, column=0, columnspan=3, pady=10)

# === 日志显示区域 ===
log_frame = ttk.LabelFrame(main_frame, text="📋 日志输出", padding=10)
log_frame.pack(fill="both", expand=True, pady=(10, 0))

log_text_widget = tk.Text(log_frame, height=12, font=("Consolas", 9), bg="black", fg="white", insertbackground="white")
log_text_widget.pack(fill="both", expand=True)

scrollbar_log = ttk.Scrollbar(log_text_widget, orient="vertical", command=log_text_widget.yview)
log_text_widget.config(yscrollcommand=scrollbar_log.set)
scrollbar_log.pack(side="right", fill="y")

# === 进度条 + 版权 ===
progress_bar = ttk.Progressbar(main_frame, mode="indeterminate")
progress_bar.pack(fill="x", pady=(10, 0))

ttk.Label(
    main_frame,
    text="© 2025 ESP32-AI 小智 | 带日志分析 | 支持删除 | 视频拼接",
    font=("微软雅黑", 8),
    foreground="#666"
).pack(pady=(10, 0))

if __name__ == "__main__":
    root.mainloop()