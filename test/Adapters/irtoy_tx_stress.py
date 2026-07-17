#!/usr/bin/env python3
import argparse
import json
import struct
import sys
import time

import serial


TX_START_SEQUENCE = bytes.fromhex("26 24 25 03")
RESET_SEQUENCE = bytes.fromhex("ff ff 00 00 00 00 00")
IRTOY_UNIT_US = 21
IRTOY_END_MARKER = 0xFFFF


DEFAULT_DURATIONS_US = [
    9000, 4500,
    560, 560, 560, 560, 560, 560, 560, 560,
    560, 560, 560, 560, 560, 560, 560, 560,
    560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560, 1690, 560, 1690, 560, 1690, 560, 1690,
    560,
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


def make_payload(durations):
    payload = b"".join(struct.pack(">H", us_to_ticks(us)) for us in durations)
    return payload + struct.pack(">H", IRTOY_END_MARKER)


def replay_once(port, payload):
    port.reset_input_buffer()
    write_exact(port, TX_START_SEQUENCE)

    sent = 0
    chunk_requests = []

    while sent < len(payload):
        request = port.read(1)
        if not request:
            return False, sent, chunk_requests, b"timeout waiting chunk request"

        count = request[0]
        chunk_requests.append(count)

        if count == ord("t"):
            tail = port.read(3)
            return False, sent, chunk_requests, b"early result: " + request + tail

        if count == 0:
            return False, sent, chunk_requests, b"invalid zero chunk request"

        chunk = payload[sent:sent + count]
        write_exact(port, chunk)
        sent += len(chunk)

    result = port.read(4)
    if len(result) != 4 or result[0] != ord("t"):
        return False, sent, chunk_requests, b"bad result: " + result

    emitted = (result[1] << 8) | result[2]
    complete = result[3] == ord("C")
    return complete and emitted == len(payload), emitted, chunk_requests, result


def main():
    parser = argparse.ArgumentParser(description="Repeat the same raw IR Toy TX frame and log adapter-level failures.")
    parser.add_argument("--device", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--file", help="JSON list of microsecond durations to replay")
    parser.add_argument("--repeat", type=int, default=50)
    parser.add_argument("--gap", type=float, default=0.5, help="seconds between TX attempts")
    parser.add_argument("--mark-adjust-us", type=int, default=0, help="added to pulse/mark durations")
    parser.add_argument("--space-adjust-us", type=int, default=0, help="added to space durations")
    parser.add_argument("--scale", type=float, default=1.0, help="multiplies all durations before mark/space adjustment")
    parser.add_argument("--reset-first", action="store_true", help="send IR Toy reset sequence before the stress run")
    args = parser.parse_args()

    durations = compensate_durations(
        load_durations(args.file),
        args.mark_adjust_us,
        args.space_adjust_us,
        args.scale,
    )
    payload = make_payload(durations)

    print(f"Durations: {len(durations)}")
    print(f"Payload bytes including 0xffff: {len(payload)}")
    print(f"First durations: {durations[:12]}")

    ok_count = 0
    with serial.Serial(args.device, args.baud, timeout=1) as port:
        time.sleep(0.5)

        if args.reset_first:
            write_exact(port, RESET_SEQUENCE)
            time.sleep(0.2)

        for index in range(1, args.repeat + 1):
            ok, emitted_or_sent, requests, result = replay_once(port, payload)
            ok_count += 1 if ok else 0
            status = "OK" if ok else "FAIL"
            print(
                f"{index:03d}/{args.repeat} {status} "
                f"emitted_or_sent={emitted_or_sent} "
                f"requests={requests} result={result!r}"
            )
            time.sleep(args.gap)

    print(f"Adapter-level OK: {ok_count}/{args.repeat}")
    return 0 if ok_count == args.repeat else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
