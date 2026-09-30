"""A model of the SDI-12 receive ISR on the `micros()` (ESP32/ESP8266/Particle) path.

It reproduces the bit-assembly arithmetic of SDI12::receiveISR() for three generations of
this library - v2.1.4, v2.3.2, and v2.3.3 - feeds each one a synthetic sensor response
with a controllable amount of edge-timing error, and reports how often the response
decodes byte-for-byte.  Run it with no arguments to print the comparison tables.

Use this before changing RX_WINDOW_FUDGE or the bit-counting logic: it makes the effect
of a window width visible without a sensor on the bench.

Timing-error models (all in microseconds):
  sigma : Gaussian ISR-latency jitter added independently to every edge timestamp
  skew  : HIGH-going edges detected `skew` late (slow rising edge on a loaded line);
          LOW-going edges on time
  baud  : sensor baud error in percent (+ = sensor runs fast, bits shorter)
"""
import random, sys

BIT = 833.333
WAITING = 0xFF

def even_parity(v):
    return bin(v & 0x7F).count("1") & 1

def frame_edges(text, baud_pct=0.0, idle_us=0.0):
    """Return a list of (time, level) pairs marking level changes for a 7E1 inverted-logic
    stream. Line idle = LOW. Start bit = HIGH. data 1 = LOW, 0 = HIGH. Stop = LOW."""
    bit = BIT / (1 + baud_pct / 100.0)
    t = 0.0
    level = 0  # LOW idle
    edges = []
    for ch in text.encode():
        bits = [0] + [(ch >> i) & 1 for i in range(7)] + [even_parity(ch)] + [1]  # start(0=HIGH), data lsb-first, parity, stop(1=LOW)
        for b in bits:
            lvl = 0 if b == 1 else 1  # inverse logic: 1 -> LOW(0), 0 -> HIGH(1)
            if lvl != level:
                edges.append((t, lvl))
                level = lvl
            t += bit
        t += idle_us
    return edges

class Rx:
    """Port of receiveISR(). mode in {'2.1.4','2.3.2','proto'}."""
    def __init__(self, mode, fudge=None, parity=None, sticky=None):
        self.mode = mode
        if mode == "2.1.4":
            self.fudge, self.parity, self.sticky, self.drop0 = 2, False, False, False
        elif mode == "2.3.2":
            self.fudge, self.parity, self.sticky, self.drop0 = 50, True, True, True
        else:  # "2.3.3"
            self.fudge, self.parity, self.sticky, self.drop0 = 416, True, True, True
        if fudge is not None: self.fudge = fudge
        if parity is not None: self.parity = parity
        if sticky is not None: self.sticky = sticky
        self.reset()

    def reset(self):
        self.rxState, self.rxMask, self.rxValue = WAITING, 1, 0
        self.prev = 0
        self.parityFailure = False
        self.out = []
        self.parity_errors = 0

    def readtime(self, t):
        if self.mode == "2.1.4":
            return int(t) >> 6          # micros()>>6, 64 us ticks
        return int(t)

    def bit_times(self, dt):
        if self.mode == "2.1.4":
            dt &= 0xFF                   # (uint8_t)(this - prev)
            return ((dt + self.fudge) * 79) >> 10
        return (dt + self.fudge) // 833

    def start_char(self):
        self.rxState, self.rxMask, self.rxValue = 0, 1, 0

    def isr(self, t, level):
        now = self.readtime(t)
        if self.mode == "2.1.4":
            # 2.1.4 only computes rxBits when NOT waiting for a start bit
            if self.rxState == WAITING:
                if level == 0: return
                self.start_char()
                self.prev = now
                return
            rxBits = self.bit_times((now - self.prev) & 0xFFFF)
        else:
            rxBits = self.bit_times((now - self.prev) & 0xFFFFFFFF)
            if self.drop0 and rxBits == 0:
                return                     # NOTE: prev NOT updated
            if self.rxState == WAITING:
                if level == 0: return
                self.start_char()
                self.prev = now
                return
            if self.mode != "2.1.4" and rxBits > 12:   # 2.3.1+: too many bits, resync
                self.rxState = WAITING
                return
        bitsLeft = 9 - self.rxState
        nextCharStarted = rxBits > bitsLeft
        bitsThisFrame = bitsLeft if nextCharStarted else rxBits
        self.rxState += bitsThisFrame
        if level == 1:
            while bitsThisFrame > 0:
                self.rxValue |= self.rxMask
                self.rxMask = (self.rxMask << 1) & 0xFF
                bitsThisFrame -= 1
            self.rxMask = (self.rxMask << 1) & 0xFF
        else:
            self.rxMask = (self.rxMask << max(bitsThisFrame - 1, 0)) & 0xFF
            self.rxValue |= self.rxMask
        if self.rxState > 7:
            rxParity = (self.rxValue >> 7) & 1
            self.rxValue &= 0x7F
            if self.parity:
                if rxParity != even_parity(self.rxValue):
                    self.parity_errors += 1
                    if self.sticky: self.parityFailure = True
                    else: self.parityFailure = True  # per-char: drop this one only
                if not self.parityFailure:
                    self.out.append(self.rxValue)
                if not self.sticky:
                    self.parityFailure = False
            else:
                self.out.append(self.rxValue)
            if level == 0 or not nextCharStarted:
                self.rxState = WAITING
            else:
                self.start_char()
        self.prev = now

def run(mode, text, sigma=0.0, skew=0.0, baud=0.0, trials=300, seed=1, **kw):
    rng = random.Random(seed)
    ok = 0
    for _ in range(trials):
        rx = Rx(mode, **kw)
        rx.prev = rx.readtime(-5000.0)  # entered LISTENING 5 ms before the response
        edges = frame_edges(text, baud_pct=baud)
        meas = []
        for (t, lvl) in edges:
            e = rng.gauss(0, sigma) if sigma else 0.0
            if lvl == 1: e += skew
            meas.append((t + 20.0 + e, lvl))   # +20us constant ISR latency
        meas.sort()
        for (t, lvl) in meas:
            rx.isr(t, lvl)
        if bytes(rx.out).decode(errors="replace") == text:
            ok += 1
    return ok / trials

if __name__ == "__main__":
    resp = "0+1234.5+0.000+0+0.0+1.23+187.6+2.45+28.93+3.14+101.32+0.734+29.10+0.5-1.2+0+1.10+0.54\r\n"
    modes = [("v2.1.4", dict(mode="2.1.4")),
             ("v2.3.2", dict(mode="2.3.2")),
             ("v2.3.2 +IGNORE_PARITY", dict(mode="2.3.2", parity=False)),
             ("v2.3.3 (fudge 416)", dict(mode="2.3.3")),
             ("v2.3.3 +IGNORE_PARITY", dict(mode="2.3.3", parity=False))]
    def table(title, sweep, key):
        print(f"\n== {title} ==  (fraction of {len(resp)}-char responses decoded perfectly)")
        print(f"{'':>8}" + "".join(f"{n:>24}" for n, _ in modes))
        for v in sweep:
            row = f"{v:>8}"
            for _, kw in modes:
                row += f"{run(text=resp, **{key: v}, **kw):>24.2f}"
            print(row)
    table("Rising-edge skew (us), no jitter", [0, 25, 50, 75, 100, 125, 150, 200, 300, 400], "skew")
    table("Gaussian ISR-latency jitter sigma (us)", [0, 10, 20, 30, 50, 75, 100, 150], "sigma")
    table("Sensor baud error (%), no jitter", [-3, -2, -1, 0, 1, 2, 3, 4], "baud")
