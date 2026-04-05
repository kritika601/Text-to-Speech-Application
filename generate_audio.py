#!/usr/bin/env python3
"""
STM32 TTS Generator using espeak-ng
Generates ~300KB compressed audio library
"""

import subprocess
import wave
import numpy as np
import os
import struct

SAMPLE_RATE = 22050
TARGET_SIZE_BYTES = 512 * 1024  # 512KB target for STM32F411RE

# Essential vocabulary
CORE_VOCABULARY = {
    # Critical words (20 words)
    'hello': 'hello', 'ready': 'ready', 'error': 'error', 'warning': 'warning',
    'system': 'system', 'sensor': 'sensor', 'temperature': 'temperature',
    'working': 'working', 'pressure': 'pressure', 'battery': 'battery',
    'degrees': 'degrees', 'celsius': 'celsius', 'percent': 'percent',
    'high': 'high', 'low': 'low', 'is': 'is', 'and': 'and', 'the': 'the',
    'on': 'on', 'off': 'off',
    
    # Numbers 0-20
    'zero': 'zero', 'one': 'one', 'two': 'two', 'three': 'three',
    'four': 'four', 'five': 'five', 'six': 'six', 'seven': 'seven',
    'eight': 'eight', 'nine': 'nine', 'ten': 'ten',
    'eleven': 'eleven', 'twelve': 'twelve', 'thirteen': 'thirteen',
    'fourteen': 'fourteen', 'fifteen': 'fifteen', 'sixteen': 'sixteen',
    'seventeen': 'seventeen', 'eighteen': 'eighteen', 'nineteen': 'nineteen',
    'twenty': 'twenty',
}

# letters for unknown words
COMPACT_LETTERS = {
    'a': 'ah',          # Short 'a' sound 
    'b': 'buh',         # 'b' sound
    'c': 'kuh',         # Hard 'c' sound  
    'd': 'duh',         # 'd' sound
    'e': 'eh',          # Short 'e' sound 
    'f': 'fuh',         # 'f' sound
    'g': 'guh',         # Hard 'g' sound
    'h': 'huh',         # 'h' sound
    'i': 'ih',          # Short 'i' sound 
    'j': 'juh',         # 'j' sound
    'k': 'kuh',         # 'k' sound 
    'l': 'luh',         # 'l' sound
    'm': 'muh',         # 'm' sound
    'n': 'nuh',         # 'n' sound
    'o': 'oh',          # Long 'o' sound 
    'p': 'puh',         # 'p' sound
    'q': 'kwuh',        # 'qu' sound
    'r': 'ruh',         # 'r' sound
    's': 'suh',         # 's' sound
    't': 'tuh',         # 't' sound
    'u': 'uh',          # Short 'u' sound 
    'v': 'vuh',         # 'v' sound
    'w': 'wuh',         # 'w' sound
    'x': 'ks',          # 'x' sound 
    'y': 'yuh',         # 'y' sound
    'z': 'zuh',         # 'z' sound
}

# ADPCM Encoder 
class AdvancedADPCM:
    def __init__(self):
        self.predicted = 0
        self.step_index = 0
        self.step_table = [
            7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
            34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
            157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
            598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
            1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
            5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
            15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
        ]
        self.index_table = [-1, -1, -1, -1, 2, 4, 6, 8]
    
    def reset(self):
        self.predicted = 0
        self.step_index = 0
    
    def encode(self, sample):
        step = self.step_table[self.step_index]
        diff = sample - self.predicted
        
        sign = 0
        if diff < 0:
            sign = 8
            diff = -diff
        
        delta = 0
        if diff >= step:
            delta |= 4
            diff -= step
        step >>= 1
        if diff >= step:
            delta |= 2
            diff -= step
        step >>= 1
        if diff >= step:
            delta |= 1
        
        code = delta | sign
        self._decode_step(code)
        return code & 0xF
    
    def _decode_step(self, code):
        step = self.step_table[self.step_index]
        diff = step >> 3
        
        if code & 4:
            diff += step
        if code & 2:
            diff += step >> 1
        if code & 1:
            diff += step >> 2
        
        if code & 8:
            self.predicted -= diff
        else:
            self.predicted += diff
        
        self.predicted = max(-32768, min(32767, self.predicted))
        self.step_index += self.index_table[code & 7]
        self.step_index = max(0, min(88, self.step_index))

def find_espeak():
    """Find espeak-ng executable on Windows"""
    possible_paths = [
        r'C:\Program Files\eSpeak NG\espeak-ng.exe',
        r'C:\Program Files (x86)\eSpeak NG\espeak-ng.exe',
        'espeak-ng',  # If in PATH
        'espeak',     # Alternative name
    ]
    
    for path in possible_paths:
        try:
            subprocess.run([path, '--version'], capture_output=True, check=True)
            print(f"Found espeak-ng at: {path}")
            return path
        except:
            continue
    
    print("ERROR: espeak-ng not found!")
    print("Please install from: https://github.com/espeak-ng/espeak-ng/releases")
    return None

def generate_audio(text, output_file, espeak_path, max_duration=2.0):
    """Generate audio using espeak-ng"""
    if not text.strip():
        return False
    
    cmd = [
        espeak_path,
        '-v', 'en',          # English voice
        '-s', '150',         # Speed
        '-a', '200',         # Amplitude
        '-w', output_file,   # Output WAV
        text
    ]
    
    try:
        subprocess.run(cmd, check=True, capture_output=True)
        print(f"✓ Generated: '{text}'")
        return True
    except Exception as e:
        print(f"✗ Failed: '{text}' - {e}")
        return False

def compress_audio(wav_file):
    """Compress WAV using ADPCM"""
    try:
        with wave.open(wav_file, 'rb') as wav:
            frames = wav.readframes(wav.getnframes())
            sample_rate = wav.getframerate()
            channels = wav.getnchannels()
            sample_width = wav.getsampwidth()
            
            # Convert to 16-bit mono
            if sample_width == 2:
                audio_data = np.frombuffer(frames, dtype=np.int16)
            else:
                audio_data = np.frombuffer(frames, dtype=np.uint8)
                audio_data = (audio_data.astype(np.int16) - 128) * 256
            
            if channels == 2:
                audio_data = audio_data[::2]
            
            # Resample to target rate
            if sample_rate != SAMPLE_RATE:
                new_length = int(len(audio_data) * SAMPLE_RATE / sample_rate)
                audio_data = np.interp(
                    np.linspace(0, len(audio_data)-1, new_length),
                    np.arange(len(audio_data)),
                    audio_data
                ).astype(np.int16)
            
            # Trim silence
            audio_data = trim_silence(audio_data)
            
            # Normalize
            max_amp = np.max(np.abs(audio_data))
            if max_amp > 0:
                audio_data = (audio_data * (20000 / max_amp) * 0.6).astype(np.int16)
            
            # ADPCM encode
            encoder = AdvancedADPCM()
            encoder.reset()
            
            compressed_nibbles = []
            for sample in audio_data:
                nibble = encoder.encode(int(sample))
                compressed_nibbles.append(nibble)
            
            # Pack nibbles
            packed_bytes = []
            for i in range(0, len(compressed_nibbles), 2):
                byte_val = compressed_nibbles[i] & 0xF
                if i + 1 < len(compressed_nibbles):
                    byte_val |= (compressed_nibbles[i + 1] & 0xF) << 4
                packed_bytes.append(byte_val)
            
            return packed_bytes, len(audio_data)
            
    except Exception as e:
        print(f"Compression error: {e}")
        return [], 0

def trim_silence(audio_data, threshold=200):
    """Remove silence from audio"""
    if len(audio_data) == 0:
        return audio_data
    
    # Find first significant sample
    start = 0
    for i, sample in enumerate(audio_data):
        if abs(sample) > threshold:
            start = max(0, i - 100)
            break
    
    # Find last significant sample
    end = len(audio_data)
    for i in range(len(audio_data) - 1, -1, -1):
        if abs(audio_data[i]) > threshold:
            end = min(len(audio_data), i + 100)
            break
    
    return audio_data[start:end]

def main():
    print("STM32 TTS Generator")
    print("=" * 50)
    
    # Find espeak
    espeak_path = find_espeak()
    if not espeak_path:
        return
    
    temp_dir = "temp_tts"
    os.makedirs(temp_dir, exist_ok=True)
    
    all_data = {}
    all_sizes = {}
    total_bytes = 0
    
    # Generate vocabulary
    print("\nGenerating vocabulary...")
    for word, text in CORE_VOCABULARY.items():
        wav_file = os.path.join(temp_dir, f"{word}.wav")
        if generate_audio(text, wav_file, espeak_path):
            compressed, orig_size = compress_audio(wav_file)
            if compressed:
                all_data[f"word_{word}"] = compressed
                all_sizes[f"word_{word}"] = orig_size
                total_bytes += len(compressed)
                print(f"  {word}: {len(compressed)} bytes")
    
    # Generate letters with PHONETIC SOUNDS
    print("\nGenerating phonetic letter sounds...")
    for letter, sound in COMPACT_LETTERS.items():
        wav_file = os.path.join(temp_dir, f"letter_{letter}.wav")
        if generate_audio(sound, wav_file, espeak_path):
            compressed, orig_size = compress_audio(wav_file)
            if compressed:
                all_data[f"letter_{letter}"] = compressed
                all_sizes[f"letter_{letter}"] = orig_size
                total_bytes += len(compressed)
                print(f"  {letter} → '{sound}': {len(compressed)} bytes")
    
    # Generate header file
    print(f"\nGenerating tts_data.h... Total: {total_bytes/1024:.1f} KB")
    
    with open('tts_data.h', 'w') as f:
        f.write('''#ifndef TTS_DATA_H
#define TTS_DATA_H

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define TTS_SAMPLE_RATE 22050

// ADPCM Decoder
typedef struct {
    int predicted_sample;
    int step_index;
} adpcm_decoder_t;

static const int step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
    1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int index_table[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static void adpcm_decoder_init(adpcm_decoder_t* decoder) {
    decoder->predicted_sample = 0;
    decoder->step_index = 0;
}

static int16_t adpcm_decode_sample(adpcm_decoder_t* decoder, uint8_t code) {
    int step = step_table[decoder->step_index];
    int diff = step >> 3;
    
    if (code & 4) diff += step;
    if (code & 2) diff += step >> 1;
    if (code & 1) diff += step >> 2;
    
    if (code & 8) {
        decoder->predicted_sample -= diff;
    } else {
        decoder->predicted_sample += diff;
    }
    
    decoder->predicted_sample = (decoder->predicted_sample < -32768) ? -32768 :
                                (decoder->predicted_sample > 32767) ? 32767 :
                                decoder->predicted_sample;
    
    decoder->step_index += index_table[code & 7];
    decoder->step_index = (decoder->step_index < 0) ? 0 :
                          (decoder->step_index > 88) ? 88 : decoder->step_index;
    
    return (int16_t)decoder->predicted_sample;
}

''')
        
        # Write compressed data
        for key, data in all_data.items():
            f.write(f'static const uint8_t tts_{key}[] = {{')
            for i, byte in enumerate(data):
                if i % 20 == 0:
                    f.write('\n    ')
                f.write(f'0x{byte:02X}')
                if i < len(data) - 1:
                    f.write(',')
            f.write(f'\n}};\n')
            f.write(f'#define TTS_{key.upper()}_SIZE {len(data)}\n')
            f.write(f'#define TTS_{key.upper()}_SAMPLES {all_sizes[key]}\n\n')
        
        f.write('#endif\n')
    
    print(f"\n✓ SUCCESS!")
    print(f"Generated tts_data.h with {total_bytes/1024:.1f} KB of audio")
    print(f"Letters now use phonetic sounds (ah, buh, kuh...) instead of names (ay, bee, see...)")
    print(f"Next: Copy tts_data.h to your STM32 project!")

if __name__ == '__main__':
    main()