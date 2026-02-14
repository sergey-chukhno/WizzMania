import math
import wave
import struct
import random

def generate_cyberpunk_track(filename, duration=30, sample_rate=44100):
    num_samples = duration * sample_rate
    audio_data = [0.0] * num_samples
    
    # 1. Drone / Bass (The Foundation)
    # C2 (65.41), G2 (98.00) - Power chord base
    drone_freqs = [65.41, 98.00]
    for i in range(num_samples):
        t = i / sample_rate
        sample = 0
        for f in drone_freqs:
            # Add some modulation to the drone
            mod = math.sin(2 * math.pi * 0.2 * t) * 0.5 + 1.0
            sample += math.sin(2 * math.pi * f * t) * 0.3 * mod
            # Add a sub-oscillator
            sample += math.sin(2 * math.pi * (f/2) * t) * 0.2
        audio_data[i] += sample

    # 2. Melody (Arpeggio)
    # C Minor scale: C, D, Eb, F, G, Ab, Bb
    # Frequencies for C4 octave: 261.63, 293.66, 311.13, 349.23, 392.00, 415.30, 466.16
    melody_notes = [261.63, 311.13, 392.00, 466.16, 392.00, 311.13] # C, Eb, G, Bb...
    note_duration = 0.25 # seconds
    samples_per_note = int(note_duration * sample_rate)
    
    current_sample = 0
    while current_sample < num_samples:
        for note_freq in melody_notes:
            if current_sample >= num_samples: break
            
            # Add some randomness to melody timing/presence
            if random.random() > 0.3: # 70% chance to play a note
                for j in range(samples_per_note):
                    if current_sample + j >= num_samples: break
                    
                    t = j / sample_rate
                    # Pluck envelope
                    env = math.exp(-5 * t)
                    # FM Synthesis for "bell/glass" cyberpunk tone
                    mod_idx = 2.0 * env
                    modulator = math.sin(2 * math.pi * note_freq * 2.0 * t)
                    carrier = math.sin(2 * math.pi * note_freq * t + mod_idx * modulator)
                    
                    # Add delay/echo effect manually
                    echo_idx = current_sample + j + int(0.3 * sample_rate)
                    if echo_idx < num_samples:
                        audio_data[echo_idx] += carrier * env * 0.2

                    audio_data[current_sample + j] += carrier * env * 0.4
            
            current_sample += samples_per_note

    # 3. Laser Effects (Randomly interspersed)
    num_lasers = int(duration / 2) # Approx one every 2 seconds
    for _ in range(num_lasers):
        start_pos = random.randint(0, num_samples - int(0.5 * sample_rate))
        laser_dur = random.uniform(0.1, 0.3)
        laser_len = int(laser_dur * sample_rate)
        
        start_freq = random.uniform(800, 2000)
        end_freq = random.uniform(100, 400)
        
        for j in range(laser_len):
            if start_pos + j >= num_samples: break
            
            progress = j / laser_len
            t = j / sample_rate
            
            # Pitch sweep
            curr_freq = start_freq * (end_freq / start_freq) ** progress
            
            # Waveform (Sawtooth-ish for grit)
            val = 0
            for h in range(1, 4):
                val += math.sin(2 * math.pi * curr_freq * h * t) / h
            
            env = (1 - progress) ** 2
            
            # Panning (simple stereo simulation by just adding to mono mix)
            audio_data[start_pos + j] += val * env * 0.3

    # 4. Write to file
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        
        # Normalize and clip
        max_val = max(abs(x) for x in audio_data)
        if max_val > 0:
            audio_data = [x / max_val for x in audio_data]
            
        for sample in audio_data:
            # Soft clip
            sample = max(min(sample, 1.0), -1.0)
            packed_sample = struct.pack('h', int(sample * 32767.0))
            wav_file.writeframes(packed_sample)

if __name__ == "__main__":
    print("Generating melodic cyberpunk track with lasers...")
    generate_cyberpunk_track("Presentation/assets/audio/cyberpunk_theme.wav")
    print("Done.")
