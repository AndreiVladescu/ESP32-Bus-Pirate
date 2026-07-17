#!/usr/bin/env python3
import argparse
import json
import struct
import sys
import time

import serial


RESET_SEQUENCE = bytes.fromhex("ff ff 00 00 00 00 00")
TX_START_SEQUENCE = bytes.fromhex("26 24 25 03")
IRTOY_UNIT_US = 21
IRTOY_END_MARKER = 0xFFFF


def us_to_ticks(us):
    ticks = int(round(us / IRTOY_UNIT_US))
    return max(1, min(0xFFFE, ticks))


def write_exact(port, data):
    written = port.write(data)
    if written != len(data):
        raise RuntimeError(f"short write: {written}/{len(data)}")


def enter_sample_mode(port):
    port.reset_input_buffer()
    write_exact(port, b"s")
    ack = port.read(3)
    if ack != b"S01":
        raise RuntimeError(f"sample mode failed: {ack!r}")


def capture_frame(port, timeout_s, min_first_us):
    enter_sample_mode(port)
    print("Press one remote key...")

    durations = []
    started = False
    deadline = time.time() + timeout_s

    while time.time() < deadline:
        data = port.read(2)
        if len(data) != 2:
            continue

        value = struct.unpack(">H", data)[0]
        if value == IRTOY_END_MARKER:
            if started:
                return durations
            continue

        us = value * IRTOY_UNIT_US
        if not started:
            if us < min_first_us:
                continue
            started = True

        durations.append(us)

    raise TimeoutError("no complete IR frame captured")


def print_capture(durations):
    print(f"Captured durations: {len(durations)}")
    print()
    print("Python list:")
    print(durations)
    print()
    print("LIRC raw block:")
    for index in range(0, len(durations), 8):
        print("  " + " ".join(str(v) for v in durations[index:index + 8]))
    print()


def compensate_durations(durations, mark_adjust_us, space_adjust_us, scale):
    compensated = []
    for index, us in enumerate(durations):
        adjusted = us * scale
        adjusted += mark_adjust_us if index % 2 == 0 else space_adjust_us
        compensated.append(max(1, int(round(adjusted))))
    return compensated


def replay_frame(port, durations):
    payload = b"".join(struct.pack(">H", us_to_ticks(us)) for us in durations)
    payload += struct.pack(">H", IRTOY_END_MARKER)

    port.reset_input_buffer()
    write_exact(port, TX_START_SEQUENCE)

    sent = 0
    while sent < len(payload):
        request = port.read(1)
        if not request:
            raise RuntimeError("device did not request a TX chunk")

        count = request[0]
        if count == ord("t"):
            raise RuntimeError("device returned TX result before full payload was sent")

        chunk = payload[sent:sent + count]
        write_exact(port, chunk)
        sent += len(chunk)

    result = port.read(4)
    if len(result) != 4 or result[0] != ord("t") or result[3] != ord("C"):
        raise RuntimeError(f"unexpected TX result: {result!r}")

    emitted = (result[1] << 8) | result[2]
    print(f"Replay OK: emitted {emitted} bytes")


def main():
    parser = argparse.ArgumentParser(description="Capture one IR Toy frame, print it, then replay it.")
    parser.add_argument("--device", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=10)
    parser.add_argument("--min-first-us", type=int, default=2000)
    parser.add_argument("--no-replay", action="store_true")
    parser.add_argument("--replay-delay", type=float, default=1.0)
    parser.add_argument("--save", help="write captured microsecond durations as a JSON list")
    parser.add_argument("--mark-adjust-us", type=int, default=0, help="added to pulse/mark durations before replay")
    parser.add_argument("--space-adjust-us", type=int, default=0, help="added to space durations before replay")
    parser.add_argument("--scale", type=float, default=1.0, help="multiplies all durations before mark/space adjustment")
    args = parser.parse_args()

    with serial.Serial(args.device, args.baud, timeout=0.5) as port:
        time.sleep(0.5)
        write_exact(port, RESET_SEQUENCE)
        time.sleep(0.1)

        durations = capture_frame(port, args.timeout, args.min_first_us)
        print_capture(durations)

        if args.save:
            with open(args.save, "w") as handle:
                json.dump(durations, handle)
                handle.write("\n")
            print(f"Saved capture to {args.save}")
            print()

        if args.no_replay:
            return 0

        replay_durations = compensate_durations(
            durations,
            args.mark_adjust_us,
            args.space_adjust_us,
            args.scale
        )
        if replay_durations != durations:
            print("Compensated replay durations:")
            print_capture(replay_durations)

        print(f"Replaying in {args.replay_delay:.1f}s...")
        time.sleep(args.replay_delay)
        replay_frame(port, replay_durations)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
