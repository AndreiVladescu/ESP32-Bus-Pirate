#!/usr/bin/env python3
import argparse
import sys
import time

import serial


RESET_SEQUENCE = bytes.fromhex("ff ff 00 00 00 00 00")
KERNEL_TX_START_SEQUENCE = bytes.fromhex("26 24 25 03")
LIRC_TX_PREFIX_SEQUENCE = bytes.fromhex("24 25 26")


def expect(port, expected, label):
    got = port.read(len(expected))
    if got != expected:
        raise RuntimeError(f"{label}: expected {expected!r}, got {got!r}")
    print(f"{label}: {got!r}")


def main():
    parser = argparse.ArgumentParser(description="USB IR Toy / LIRC adapter smoke test")
    parser.add_argument("--device", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.device, args.baud, timeout=1) as port:
        time.sleep(0.5)

        port.reset_input_buffer()
        port.write(b"v")
        expect(port, b"V222", "version")

        port.write(b"s")
        expect(port, b"S01", "sample")

        port.write(RESET_SEQUENCE)
        time.sleep(0.1)

        port.write(b"s")
        expect(port, b"S01", "sample-after-reset")

        port.write(KERNEL_TX_START_SEQUENCE)
        chunk = port.read(1)
        if not chunk or chunk[0] < 2 or chunk[0] > 64:
            raise RuntimeError(f"kernel tx chunk request: invalid {chunk!r}")
        print(f"kernel tx chunk request: {chunk[0]}")

        port.write(bytes.fromhex("ff ff"))
        expect(port, b"t\x00\x02", "kernel-tx-count")
        expect(port, b"C", "kernel-tx-complete")

        port.write(b"s")
        expect(port, b"S01", "sample-after-kernel-tx")

        port.write(LIRC_TX_PREFIX_SEQUENCE)
        time.sleep(0.1)
        port.write(bytes.fromhex("31 ff ef 30 00 10"))
        time.sleep(0.05)
        port.write(b"\x03")
        chunk = port.read(1)
        if not chunk or chunk[0] < 2 or chunk[0] > 64:
            raise RuntimeError(f"lirc tx chunk request: invalid {chunk!r}")
        print(f"lirc tx chunk request: {chunk[0]}")

        port.write(bytes.fromhex("ff ff"))
        chunk = port.read(1)
        if not chunk or chunk[0] < 2 or chunk[0] > 64:
            raise RuntimeError(f"lirc final chunk request: invalid {chunk!r}")
        print(f"lirc final chunk request: {chunk[0]}")
        expect(port, b"t\x00\x02", "lirc-tx-count")
        expect(port, b"C", "lirc-tx-complete")

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
