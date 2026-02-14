import wave
import math
import struct
import random

def generate_laser_sound(filename, duration=0.3, sample_rate=44100):
    num_samples = int(duration * sample_rate)
    
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)  # 2 bytes per sample (16-bit)
        wav_file.setframerate(sample_rate)
        
        for i in range(num_samples):
            t = float(i) / sample_rate
            
            # Frequency sweep from high to low (classic laser pew)
            # Start at 880Hz, drop to 110Hz exponentially
            freq = 880.0 * (0.1 ** (t / duration))
            
            # Generate sine wave
            value = math.sin(2.0 * math.pi * freq * t)
            
            # Add some "buzz" (square wave harmonic)
            value += 0.5 * (1.0 if math.sin(2.0 * math.pi * freq * t) > 0 else -1.0)
            
            # Envelope (Attack, Decay)
            if t < 0.05:
                envelope = t / 0.05
            else:
                envelope = 1.0 - ((t - 0.05) / (duration - 0.05))
            
            # Apply envelope
            sample = value * envelope * 0.5 # Scale amplitude
            
            # Clamp and convert to 16-bit integer
            sample = max(-1.0, min(1.0, sample))
            packed_value = struct.pack('<h', int(sample * 32767.0))
            wav_file.writeframes(packed_value)

if __name__ == "__main__":
    generate_laser_sound("assets/audio/laser_shoot.wav")
    print("Generated assets/audio/laser_shoot.wav")
