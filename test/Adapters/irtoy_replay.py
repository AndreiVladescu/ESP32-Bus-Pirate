#!/usr/bin/env python3
import argparse
import json
import struct
import sys
import time

import serial


TX_START_SEQUENCE = bytes.fromhex("26 24 25 03")
IRTOY_UNIT_US = 21
IRTOY_END_MARKER = 0xFFFF


DEFAULT_DURATIONS_US = [
    8946, 4536, 504, 630, 504, 630, 546, 567,
    525, 630, 504, 609, 504, 630, 504, 1722,
    567, 588, 504, 1722, 546, 1701, 546, 1701,
    504, 1743, 546, 609, 504, 1722, 567, 567,
    525, 1743, 504, 1722, 504, 1722, 525, 630,
    483, 630, 504, 630, 504, 609, 504, 630,
    504, 609, 504, 630, 525, 588, 525, 1722,
    504, 1722, 504, 1743, 525, 1722, 525, 1701,
    525, 1722, 546,
]


def us_to_ticks(us):
    ticks = int(round(us / IRTOY_UNIT_US))
    return max(1, min(0xFFFE, ticks))


def write_exact(port, data):
    written = port.write(data)
    if written != len(data):
        raise RuntimeError(f"short write: {written}/{len(data)}")


def load_durations(path):
    if path is None:
        return DEFAULT_DURATIONS_US

    with open(path) as handle:
        data = json.load(handle)

    if not isinstance(data, list) or not all(isinstance(v, int) for v in data):
        raise ValueError("capture file must be a JSON list of integer durations in microseconds")

    return data


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

    return (result[1] << 8) | result[2]


def main():
    parser = argparse.ArgumentParser(description="Replay a raw IR Toy duration sequence.")
    parser.add_argument("--device", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--file", help="JSON list of microsecond durations to replay")
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--gap", type=float, default=0.5, help="seconds between repeats")
    parser.add_argument("--mark-adjust-us", type=int, default=0, help="added to pulse/mark durations")
    parser.add_argument("--space-adjust-us", type=int, default=0, help="added to space durations")
    parser.add_argument("--scale", type=float, default=1.0, help="multiplies all durations before mark/space adjustment")
    args = parser.parse_args()

    durations = compensate_durations(
        load_durations(args.file),
        args.mark_adjust_us,
        args.space_adjust_us,
        args.scale
    )
    print(f"Replaying {len(durations)} durations")
    print(durations)

    with serial.Serial(args.device, args.baud, timeout=1) as port:
        time.sleep(0.5)
        for index in range(args.repeat):
            emitted = replay_frame(port, durations)
            print(f"Replay {index + 1}/{args.repeat}: emitted {emitted} bytes")
            if index + 1 < args.repeat:
                time.sleep(args.gap)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
