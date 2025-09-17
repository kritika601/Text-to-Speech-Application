from gtts import gTTS
import os

def generate_tts(text):
    tts = gTTS(text=text, lang="en")
    # Always save with the same name
    output_path = "tts.mp3"  
    tts.save(output_path)
    print(f"TTS generated and saved as {output_path}")

if __name__ == "__main__":
    user_input = input("Enter text: ")
    generate_tts(user_input)

