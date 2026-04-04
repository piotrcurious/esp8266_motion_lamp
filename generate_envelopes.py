def generate_envelope_0():
    # Smooth fade in (1s/10 steps), steady (5s/1 step), smooth fade out (1s/10 steps)
    steps = []
    for i in range(1, 11):
        steps.append((int(i * 25.5), 100))
    steps.append((255, 5000))
    for i in range(9, -1, -1):
        steps.append((int(i * 25.5), 100))
    while len(steps) < 60:
        steps.append((0, 0))
    return steps, 10 # Loop to steady (index 10)

def generate_envelope_1():
    # 3 Pulses. Each pulse: 0.5s up, 0.5s down.
    steps = []
    for _ in range(3):
        for i in range(1, 6):
            steps.append((int(i * 51), 100))
        for i in range(4, -1, -1):
            steps.append((int(i * 51), 100))
    while len(steps) < 60:
        steps.append((0, 0))
    return steps, 0 # Loop to start

def generate_envelope_2():
    # Double strobe. 0.1s on, 0.1s off, 0.1s on, 0.7s off.
    steps = []
    for _ in range(10): # 10 repetitions
        steps.append((255, 100))
        steps.append((0, 100))
        steps.append((255, 100))
        steps.append((0, 700))
    while len(steps) < 60:
        steps.append((0, 0))
    return steps, 0 # Loop to start

def generate_envelope_3():
    # Night light. Dim (32/255) for 30s.
    steps = [(32, 30000)]
    while len(steps) < 60:
        steps.append((0, 0))
    return steps, 0

def generate_envelope_4():
    # Rapid alert. 0.2s on, 0.2s off.
    steps = []
    for _ in range(30):
        steps.append((255, 200))
        steps.append((0, 200))
    return steps, 0

envelopes = [
    generate_envelope_0(),
    generate_envelope_1(),
    generate_envelope_2(),
    generate_envelope_3(),
    generate_envelope_4()
]

print("const Envelope envelopes[NUM_ENVELOPES] PROGMEM = {")
for i, (steps, loop) in enumerate(envelopes):
    print("  {")
    print("    {")
    step_strings = [f"{{{b}, {d}}}" for b, d in steps]
    for j in range(0, 60, 5):
        print("      " + ", ".join(step_strings[j:j+5]) + ("," if j < 55 else ""))
    print("    },")
    print(f"    {loop}")
    print("  }" + ("," if i < 4 else ""))
print("};")
