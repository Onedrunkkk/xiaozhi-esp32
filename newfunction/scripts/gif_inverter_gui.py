import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import Image, ImageSequence
from moviepy.editor import VideoFileClip, concatenate_videoclips
import os
import threading

class MultimediaToolApp:
    def __init__(self, root):
        self.root = root
        self.root.title("RGBO 多媒体工具 v3.0")
        self.root.geometry("650x400")
        self.root.resizable(False, False)

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # 创建两个选项卡
        self.gif_frame = tk.Frame(self.notebook)
        self.video_frame = tk.Frame(self.notebook)

        self.notebook.add(self.gif_frame, text="RGBO GIF反色")
        self.notebook.add(self.video_frame, text="🎬 视频拼接")

        # 初始化两个功能
        self.setup_gif_tab()
        self.setup_video_tab()

    # ==================== GIF 反色功能 ====================
    def setup_gif_tab(self):
        frame = self.gif_frame
        tk.Label(frame, text="RGBO 反色GIF工具", font=("微软雅黑", 14, "bold")).grid(row=0, column=0, columnspan=4, pady=20)

        self.gif_input_paths = []
        self.gif_output_dir = tk.StringVar(value="")
        self.gif_progress = tk.DoubleVar(value=0)

        # 输入文件列表
        tk.Label(frame, text="输入GIF：").grid(row=1, column=0, sticky="w", pady=5)
        self.gif_listbox = tk.Listbox(frame, height=6, width=50)
        self.gif_listbox.grid(row=1, column=1, columnspan=2, padx=5, pady=5, sticky="w")

        btn_frame1 = tk.Frame(frame)
        btn_frame1.grid(row=1, column=3, padx=5)
        tk.Button(btn_frame1, text="添加文件", command=self.add_gif_files).pack(fill=tk.X, pady=2)
        tk.Button(btn_frame1, text="清空列表", command=self.clear_gif_list).pack(fill=tk.X, pady=2)

        # 输出目录
        tk.Label(frame, text="输出目录：").grid(row=2, column=0, sticky="w", pady=5)
        tk.Entry(frame, textvariable=self.gif_output_dir, width=50, state="readonly").grid(row=2, column=1, padx=5, pady=5)
        tk.Button(frame, text="选择目录", command=self.select_gif_output_dir).grid(row=2, column=3, padx=5)

        # 进度条
        tk.Label(frame, text="处理进度：").grid(row=3, column=0, sticky="w", pady=10)
        ttk.Progressbar(frame, variable=self.gif_progress, maximum=100).grid(row=3, column=1, columnspan=2, sticky="ew", padx=5, pady=10)
        self.gif_progress_label = tk.Label(frame, text="等待处理...")
        self.gif_progress_label.grid(row=3, column=3, padx=5)

        # 按钮
        btn_frame2 = tk.Frame(frame)
        btn_frame2.grid(row=4, column=0, columnspan=4, pady=10)
        tk.Button(btn_frame2, text="打开输出文件夹", command=self.open_gif_output_folder).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame2, text="开始批量处理", width=15, bg="lightgreen", font=("微软雅黑", 9, "bold"),
                  command=self.start_gif_processing).pack(side=tk.LEFT, padx=5)

    def add_gif_files(self):
        paths = filedialog.askopenfilenames(title="选择GIF文件", filetypes=[("GIF files", "*.gif")])
        if paths:
            self.gif_input_paths.extend(paths)
            self.update_gif_listbox()
            if not self.gif_output_dir.get():
                self.gif_output_dir.set(os.path.dirname(paths[0]))

    def update_gif_listbox(self):
        self.gif_listbox.delete(0, tk.END)
        for path in self.gif_input_paths:
            self.gif_listbox.insert(tk.END, os.path.basename(path))

    def clear_gif_list(self):
        self.gif_input_paths.clear()
        self.gif_listbox.delete(0, tk.END)
        self.gif_progress.set(0)
        self.gif_progress_label.config(text="等待处理...")

    def select_gif_output_dir(self):
        directory = filedialog.askdirectory()
        if directory:
            self.gif_output_dir.set(directory)

    def open_gif_output_folder(self):
        d = self.gif_output_dir.get()
        if d and os.path.exists(d):
            os.startfile(d)
        else:
            messagebox.showwarning("警告", "输出目录不存在！")

    def start_gif_processing(self):
        if not self.gif_input_paths:
            messagebox.showwarning("警告", "请添加GIF文件！")
            return
        if not self.gif_output_dir.get():
            messagebox.showwarning("警告", "请选择输出目录！")
            return
        thread = threading.Thread(target=self.batch_process_gif, daemon=True)
        thread.start()

    def batch_process_gif(self):
        total = len(self.gif_input_paths)
        self.gif_progress.set(0)
        self.gif_progress_label.config(text="开始处理...")

        for i, path in enumerate(self.gif_input_paths):
            try:
                base = os.path.basename(path)
                name, ext = os.path.splitext(base)
                output_path = os.path.join(self.gif_output_dir.get(), f"{name}_inverted{ext}")
                self.invert_gif(path, output_path)

                progress = (i + 1) / total * 100
                self.root.after(10, lambda p=progress: self.gif_progress.set(p))
                self.root.after(10, lambda t=f"完成: {base}": self.gif_progress_label.config(text=t))
            except Exception as e:
                msg = f"错误: {os.path.basename(path)} - {str(e)}"
                self.root.after(10, lambda t=msg: self.gif_progress_label.config(text=t))
                print(msg)

        self.root.after(10, lambda: self.gif_progress_label.config(text="✅ 全部完成！"))
        self.root.after(1000, lambda: messagebox.showinfo("完成", "GIF处理完毕！"))

    def invert_gif(self, input_path, output_path):
        with Image.open(input_path) as im:
            frames, durations = [], []
            for frame in ImageSequence.Iterator(im):
                durations.append(frame.info.get('duration', 100))
                rgb = frame.convert('RGB') if frame.mode != 'RGB' else frame
                inverted = rgb.point(lambda x: 255 - x)
                if 'transparency' in frame.info:
                    inverted.info['transparency'] = 255 - frame.info['transparency']
                frames.append(inverted)
            frames[0].save(
                output_path, save_all=True, append_images=frames[1:], duration=durations,
                loop=im.info.get('loop', 0), disposal=im.info.get('disposal', 2),
                transparency=frames[0].info.get('transparency'), optimize=False
            )

    # ==================== 视频拼接功能 ====================
    def setup_video_tab(self):
        frame = self.video_frame
        tk.Label(frame, text="🎬 视频拼接工具（MP4）", font=("微软雅黑", 14, "bold")).grid(row=0, column=0, columnspan=4, pady=20)

        self.video_input_paths = []
        self.video_output_path = tk.StringVar(value="")
        self.video_progress = tk.DoubleVar(value=0)

        # 输入列表
        tk.Label(frame, text="视频文件：").grid(row=1, column=0, sticky="w", pady=5)
        self.video_listbox = tk.Listbox(frame, height=6, width=50)
        self.video_listbox.grid(row=1, column=1, columnspan=2, padx=5, pady=5, sticky="w")

        btn_frame1 = tk.Frame(frame)
        btn_frame1.grid(row=1, column=3, padx=5)
        tk.Button(btn_frame1, text="添加视频", command=self.add_video_files).pack(fill=tk.X, pady=2)
        tk.Button(btn_frame1, text="上移", command=self.move_up).pack(fill=tk.X, pady=2)
        tk.Button(btn_frame1, text="下移", command=self.move_down).pack(fill=tk.X, pady=2)
        tk.Button(btn_frame1, text="清空", command=self.clear_video_list).pack(fill=tk.X, pady=2)

        # 输出文件
        tk.Label(frame, text="输出文件：").grid(row=2, column=0, sticky="w", pady=5)
        tk.Entry(frame, textvariable=self.video_output_path, width=50, state="readonly").grid(row=2, column=1, padx=5, pady=5)
        tk.Button(frame, text="选择路径", command=self.select_video_output).grid(row=2, column=3, padx=5)

        # 进度条
        tk.Label(frame, text="拼接进度：").grid(row=3, column=0, sticky="w", pady=10)
        ttk.Progressbar(frame, variable=self.video_progress, maximum=100).grid(row=3, column=1, columnspan=2, sticky="ew", padx=5, pady=10)
        self.video_progress_label = tk.Label(frame, text="等待拼接...")
        self.video_progress_label.grid(row=3, column=3, padx=5)

        # 按钮
        btn_frame2 = tk.Frame(frame)
        btn_frame2.grid(row=4, column=0, columnspan=4, pady=10)
        tk.Button(btn_frame2, text="打开文件夹", command=self.open_video_output_folder).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame2, text="开始拼接", width=15, bg="lightblue", font=("微软雅黑", 9, "bold"),
                  command=self.start_concatenate).pack(side=tk.LEFT, padx=5)

    def add_video_files(self):
        paths = filedialog.askopenfilenames(title="选择MP4视频", filetypes=[("MP4 files", "*.mp4"), ("All", "*.*")])
        if paths:
            self.video_input_paths.extend(paths)
            self.update_video_listbox()
            if not self.video_output_path.get():
                dir_name = os.path.dirname(paths[0])
                self.video_output_path.set(os.path.join(dir_name, "merged_video.mp4"))

    def update_video_listbox(self):
        self.video_listbox.delete(0, tk.END)
        for path in self.video_input_paths:
            self.video_listbox.insert(tk.END, os.path.basename(path))

    def move_up(self):
        selected = self.video_listbox.curselection()
        if selected and selected[0] > 0:
            idx = selected[0]
            self.video_input_paths[idx], self.video_input_paths[idx-1] = self.video_input_paths[idx-1], self.video_input_paths[idx]
            self.update_video_listbox()
            self.video_listbox.selection_set(idx - 1)

    def move_down(self):
        selected = self.video_listbox.curselection()
        if selected and selected[0] < len(self.video_input_paths) - 1:
            idx = selected[0]
            self.video_input_paths[idx], self.video_input_paths[idx+1] = self.video_input_paths[idx+1], self.video_input_paths[idx]
            self.update_video_listbox()
            self.video_listbox.selection_set(idx + 1)

    def clear_video_list(self):
        self.video_input_paths.clear()
        self.video_listbox.delete(0, tk.END)
        self.video_progress.set(0)
        self.video_progress_label.config(text="等待拼接...")

    def select_video_output(self):
        path = filedialog.asksaveasfilename(
            title="保存合并后的视频",
            defaultextension=".mp4",
            filetypes=[("MP4 files", "*.mp4")]
        )
        if path:
            self.video_output_path.set(path)

    def open_video_output_folder(self):
        path = self.video_output_path.get()
        if path:
            folder = os.path.dirname(path)
            if os.path.exists(folder):
                os.startfile(folder)

    def start_concatenate(self):
        if len(self.video_input_paths) < 2:
            messagebox.showwarning("警告", "请至少添加两个视频文件！")
            return
        if not self.video_output_path.get():
            messagebox.showwarning("警告", "请设置输出文件路径！")
            return
        thread = threading.Thread(target=self.concatenate_videos, daemon=True)
        thread.start()

    def concatenate_videos(self):
        clips = []
        total = len(self.video_input_paths)
        self.video_progress.set(0)
        self.video_progress_label.config(text="加载视频...")

        try:
            for i, path in enumerate(self.video_input_paths):
                self.root.after(10, lambda t=f"加载: {os.path.basename(path)}": self.video_progress_label.config(text=t))
                clip = VideoFileClip(path)
                clips.append(clip)

                progress = (i + 1) / total * 50  # 前50%用于加载
                self.root.after(10, lambda p=progress: self.video_progress.set(p))

            # 拼接
            self.root.after(10, lambda: self.video_progress_label.config(text="正在拼接视频..."))
            final_clip = concatenate_videoclips(clips, method="compose")

            # 写入文件（后50%进度）
            self.root.after(10, lambda: self.video_progress_label.config(text="正在写入文件..."))
            final_clip.write_videofile(
                self.video_output_path.get(),
                codec="libx264",
                audio_codec="aac",
                temp_audiofile="temp-audio.m4a",
                remove_temp=True,
                fps=24
            )

            # 清理资源
            for clip in clips:
                clip.close()

            self.root.after(10, lambda: self.video_progress.set(100))
            self.root.after(10, lambda: self.video_progress_label.config(text="✅ 拼接完成！"))
            self.root.after(1000, lambda: messagebox.showinfo("完成", f"视频已保存：\n{self.video_output_path.get()}"))

        except Exception as e:
            self.root.after(10, lambda: self.video_progress_label.config(text=f"❌ 错误: {str(e)}"))
            messagebox.showerror("拼接失败", str(e))


# === 运行 ===
if __name__ == "__main__":
    root = tk.Tk()
    app = MultimediaToolApp(root)
    root.mainloop()