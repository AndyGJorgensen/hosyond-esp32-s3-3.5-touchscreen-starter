#!/usr/bin/env python3
"""Generate sd_card/audio/chime.wav: a 3-note bell chime (C6, E6, G6) for testing the speaker.
Copy the sd_card/ folder contents to the root of a FAT32 microSD card."""
import math, os, struct, wave

RATE = 22050
NOTES = [(1046.50, 0.00), (1318.51, 0.18), (1567.98, 0.36)]  # Hz, start time (s)
LENGTH = 2.2  # seconds

def bell(t, f):
    # Fundamental plus inharmonic partials, each decaying at its own rate
    s = 0.0
    for mult, amp, decay in [(1.0, 1.0, 2.2), (2.76, 0.35, 4.0), (5.40, 0.15, 7.0)]:
        s += amp * math.sin(2 * math.pi * f * mult * t) * math.exp(-decay * t)
    attack = min(1.0, t / 0.004)  # 4 ms fade-in, no click
    return s * attack

samples = []
for n in range(int(RATE * LENGTH)):
    t = n / RATE
    v = sum(bell(t - start, f) for f, start in NOTES if t >= start)
    fade = min(1.0, (LENGTH - t) / 0.05)  # fade out the last 50 ms
    samples.append(v * fade)

peak = max(abs(s) for s in samples)
out = os.path.join(os.path.dirname(__file__), "..", "sd_card", "audio", "chime.wav")
os.makedirs(os.path.dirname(out), exist_ok=True)
with wave.open(out, "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(RATE)
    w.writeframes(b"".join(struct.pack("<h", int(s / peak * 0.8 * 32767)) for s in samples))
print("wrote", os.path.normpath(out))
