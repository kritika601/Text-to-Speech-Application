import tkinter as tk
from tkinter import messagebox
from gtts import gTTS
import os

# Path where tts.mp3 is saved (must be same folder where HTTP server runs)
OUTPUT_FILE = "tts.mp3"

def generate_speech():
    text = text_entry.get("1.0", tk.END).strip()
    if not text:
        messagebox.showwarning("Empty Input", "Please enter some text.")
        return

    try:
        # Generate TTS
        tts = gTTS(text=text, lang="en")
        tts.save(OUTPUT_FILE)

        messagebox.showinfo("Success", f"Speech generated → {OUTPUT_FILE}\nESP32 processing...please wait.")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to generate speech:\n{str(e)}")

# GUI setup
root = tk.Tk()
root.title("ESP32 Text-to-Speech Control")
root.geometry("400x300")

label = tk.Label(root, text="Enter text for speech:", font=("Times New Roman", 20, "bold", "italic"))
label.pack(pady=10)

text_entry = tk.Text(root, height=5, width=40)
text_entry.pack(pady=10)

generate_button = tk.Button(root, text="Generate Speech", command=generate_speech, bg="#4CAF50", fg="white", font=("Roboto", 12, "bold"))
generate_button.pack(pady=20)

root.mainloop()
