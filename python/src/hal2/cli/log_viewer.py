import dataclasses
import argparse
import pathlib
import json
import sys
import serial
import struct

import colorama

from hal2.logging.spec_json import LogSpecJson
from hal2.logging.spec import LoggingSpec


@dataclasses.dataclass
class FrameHeader:
    """Log frame header."""

    timestamp: int
    """Log timestamp, in milliseconds."""
    level: int
    """Log level."""
    module_id: int
    """Log message module ID."""
    message_id: int
    """Log message ID."""
    payload_len: int
    """Log payload length."""


@dataclasses.dataclass
class FrameFooter:
    """Log frame footer."""

    crc: int
    """Frame CRC."""


@dataclasses.dataclass
class Frame:
    """Log frame."""

    header: FrameHeader
    """Frame header."""
    payload: bytes
    """Frame payload bytes."""
    footer: FrameFooter
    """Frame footer."""


_LEVEL_COLORS = {
    10: colorama.Fore.WHITE + colorama.Style.DIM,  # TRACE
    20: colorama.Fore.WHITE,  # DEBUG
    30: colorama.Fore.BLUE,  # INFO
    40: colorama.Fore.YELLOW,  # WARN
    50: colorama.Fore.RED,  # ERROR
    60: colorama.Fore.RED + colorama.Style.BRIGHT,  # FATAL
}

_LEVEL_VALUE_COLORS = {
    10: colorama.Fore.WHITE,  # TRACE
    20: colorama.Fore.WHITE + colorama.Style.BRIGHT,  # DEBUG
    30: colorama.Fore.BLUE + colorama.Style.BRIGHT,  # INFO
    40: colorama.Fore.YELLOW + colorama.Style.BRIGHT,  # WARN
    50: colorama.Fore.RED + colorama.Style.BRIGHT,  # ERROR
    60: colorama.Fore.WHITE + colorama.Back.RED + colorama.Style.BRIGHT,  # FATAL
}


def read_frame(ser: serial.Serial) -> Frame:
    """
    Reads a log frame from the serial.

    Args:
        ser: Serial to read from.

    Returns:
        Decoded frame.
    """

    # Read header
    header_fmt = "<IBHBB"
    header_n_bytes = struct.calcsize(header_fmt)
    header_bytes = ser.read(header_n_bytes)

    # Decode header.
    timestamp, level, module_id, message_id, payload_len = struct.unpack(header_fmt, header_bytes)
    header = FrameHeader(
        timestamp=timestamp,
        level=level,
        module_id=module_id,
        message_id=message_id,
        payload_len=payload_len,
    )

    # Read payload if present.
    if header.payload_len > 0:
        payload_bytes = ser.read(header.payload_len)
    else:
        payload_bytes = bytes()

    # Read footer.
    footer_fmt = "<H"
    footer_n_bytes = struct.calcsize(footer_fmt)
    footer_bytes = ser.read(footer_n_bytes)

    # Decode footer
    (crc,) = struct.unpack(footer_fmt, footer_bytes)
    footer = FrameFooter(crc=crc)

    return Frame(header=header, payload=payload_bytes, footer=footer)


def format_ts(timestamp_ms: int) -> str:
    SECOND = 1000
    MINUTE = 60 * SECOND
    HOUR = 60 * MINUTE

    hours = timestamp_ms // HOUR
    timestamp_ms -= hours * HOUR

    minutes = timestamp_ms // MINUTE
    timestamp_ms -= minutes * MINUTE

    seconds = timestamp_ms // SECOND
    timestamp_ms -= seconds * SECOND

    return f"{hours:02}:{minutes:02}:{seconds:02}.{timestamp_ms:03}"



def print_message(timestamp: int, level: int, module: str, message: str):
    level_names = {
        10: "TRACE",
        20: "DEBUG",
        30: "INFO",
        40: "WARN",
        50: "ERROR",
        60: "FATAL"
    }

    lvl = f"{level_names.get(level, "???"):<5}"

    if level in _LEVEL_COLORS:
        lvl = f"{_LEVEL_COLORS[level]}{lvl}{colorama.Style.RESET_ALL}"

    print(f"{format_ts(timestamp)} [{lvl}] {module}: {message}")


def main():
    # Define and parse command-line arguments.
    parser = argparse.ArgumentParser(description="Read and log binary log messages")
    parser.add_argument("--log-spec", type=pathlib.Path, help="Path to the JSON log specification file", required=True)
    parser.add_argument("--port", type=str, help="Serial port to read from", required=True)
    args = parser.parse_args()

    # Read logging spec definition.
    with args.log_spec.open("r") as fin:
        json_spec: LogSpecJson = json.load(fin)

    # Create specification
    spec = LoggingSpec(json_spec)

    # Open serial and start reading messages.
    with serial.Serial(args.port, timeout=None) as ser:
        while True:
            # Start reading until we receive the start byte (b"L")
            start_byte = ser.read(1)
            if start_byte != b"L":
                continue

            try:
                frame = read_frame(ser)

                msg_pre = _LEVEL_COLORS[frame.header.level]
                val_pre = colorama.Style.RESET_ALL + _LEVEL_VALUE_COLORS[frame.header.level]
                val_post = colorama.Style.RESET_ALL + _LEVEL_COLORS[frame.header.level]

                module, msg = spec.decode(
                    module_id=frame.header.module_id,
                    msg_id=frame.header.message_id,
                    payload=frame.payload,
                    wrap_value=(val_pre, val_post),
                )

                msg = f"{msg_pre}{msg}{colorama.Style.RESET_ALL}"

                print_message(frame.header.timestamp, frame.header.level, module, msg)
            except Exception as e:
                exc_msg = f"{_LEVEL_COLORS[60]}{e}{colorama.Style.RESET_ALL}"
                print_message(0, 60, "Logger", exc_msg)


if __name__ == "__main__":
    sys.exit(main())
