#!/usr/bin/env python3
import argparse
import sys
import time

import serial


def eol_bytes(name):
    values = {
        "lf": b"\n",
        "cr": b"\r",
        "crlf": b"\r\n",
        "none": b"",
    }
    try:
        return values[name]
    except KeyError as exc:
        raise argparse.ArgumentTypeError("expected lf, cr, crlf, or none") from exc


def read_lines(port, deadline):
    lines = []
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            lines.append(line)
    return lines


def send_command(port, command, eol, timeout=2.0, stop=None):
    port.reset_input_buffer()
    port.write(command.encode("ascii") + eol)
    port.flush()

    deadline = time.monotonic() + timeout
    lines = []
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue
        lines.append(line)
        if stop is None:
            return lines
        if stop(line, lines):
            return lines
    return lines


def has_line(lines, predicate):
    return any(predicate(line) for line in lines)


def first_line(lines, predicate):
    for line in lines:
        if predicate(line):
            return line
    return None


def run_test(args):
    eol = eol_bytes(args.eol)
    report = []
    passed = True
    rx_passed = False

    with serial.Serial(args.port, args.baud, timeout=0.2) as port:
        time.sleep(0.5)
        port.reset_input_buffer()

        version = send_command(port, "V", eol, stop=lambda line, _: line.startswith("V "))
        version_ok = has_line(version, lambda line: line == "V 1.0 ESP32-BitPirate SubGHz Raw CDC")
        report.append(("V version", version_ok, version))
        passed = passed and version_ok

        help_lines = send_command(port, "?", eol, stop=lambda line, _: line == "OK")
        help_ok = has_line(help_lines, lambda line: line == "OK") and has_line(help_lines, lambda line: "F433.920" in line)
        report.append(("? help", help_ok, help_lines))
        passed = passed and help_ok

        freq_cmd = f"F{args.freq:.3f}"
        freq_lines = send_command(port, freq_cmd, eol, stop=lambda line, _: line in ("OK", "ERR:FREQ", "ERR:CC1101"))
        freq_ok = has_line(freq_lines, lambda line: line == "OK")
        report.append((freq_cmd, freq_ok, freq_lines))
        passed = passed and freq_ok

        rssi_lines = send_command(port, "R", eol, stop=lambda line, _: line.startswith("RSSI:") or line.startswith("ERR:"))
        rssi_line = first_line(rssi_lines, lambda line: line.startswith("RSSI:"))
        rssi_ok = False
        if rssi_line is not None:
            try:
                int(rssi_line.split(":", 1)[1])
                rssi_ok = True
            except ValueError:
                rssi_ok = False
        report.append(("R RSSI", rssi_ok, rssi_lines))
        passed = passed and rssi_ok

        rx_on_lines = send_command(port, "X21", eol, stop=lambda line, _: line in ("OK", "ERR:CC1101", "ERR:X"))
        rx_on_ok = has_line(rx_on_lines, lambda line: line == "OK")
        report.append(("X21 RX on", rx_on_ok, rx_on_lines))
        passed = passed and rx_on_ok

        x_state_lines = send_command(port, "X", eol, stop=lambda line, _: line.startswith("X:") or line.startswith("ERR:"))
        x_state_ok = has_line(x_state_lines, lambda line: line == "X:21")
        report.append(("X RX state", x_state_ok, x_state_lines))
        passed = passed and x_state_ok

        led_lines = send_command(port, "l00", eol, stop=lambda line, _: line in ("OK", "ERR:LED"))
        led_ok = has_line(led_lines, lambda line: line == "OK")
        report.append(("l00 LED no-op", led_ok, led_lines))
        passed = passed and led_ok

        boot_lines = send_command(port, "B00", eol, stop=lambda line, _: line in ("OK", "ERR:BOOT"))
        boot_ok = has_line(boot_lines, lambda line: line == "OK")
        report.append(("B00 reset no-op", boot_ok, boot_lines))
        passed = passed and boot_ok

        listen_deadline = time.monotonic() + args.listen
        listen_lines = read_lines(port, listen_deadline)
        raw_lines = [line for line in listen_lines if line.startswith("RAW:")]
        rx_passed = len(raw_lines) > 0
        report.append((f"RAW listen {args.listen:.1f}s", rx_passed, raw_lines[:5]))

        rx_off_lines = send_command(port, "X00", eol, stop=lambda line, _: line in ("OK", "ERR:X"))
        rx_off_ok = has_line(rx_off_lines, lambda line: line == "OK")
        report.append(("X00 RX off", rx_off_ok, rx_off_lines))
        passed = passed and rx_off_ok

    print("SubGHz Raw CDC adapter test")
    print(f"port={args.port} baud={args.baud} freq={args.freq:.3f}MHz listen={args.listen:.1f}s eol={args.eol}")
    print()

    for label, ok, lines in report:
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {label}")
        for line in lines:
            print(f"  {line}")

    print()
    if passed and rx_passed:
        print("RESULT: PASS")
        return 0

    if passed and not rx_passed:
        print("RESULT: PARTIAL PASS")
        print("Command path passed, but no RAW frame was received.")
        print("Check frequency, GDO0 pin, CC1101 wiring, modulation/profile, antenna, remote battery, and that the remote is transmitting.")
        return 2

    print("RESULT: FAIL")
    return 1


def main():
    parser = argparse.ArgumentParser(description="ESP32-Bit Pirate SubGHz Raw CDC adapter smoke test")
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=38400)
    parser.add_argument("--freq", type=float, default=433.920)
    parser.add_argument("--listen", type=float, default=15.0)
    parser.add_argument("--eol", default="lf", choices=("lf", "cr", "crlf", "none"))
    return run_test(parser.parse_args())


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130)
    except Exception as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(1)
