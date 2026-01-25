import struct
from typing import TypedDict, Optional
import argparse
import sys

import json
import pathlib

import colorama
import serial


_HEADER_SPEC = "<IBHBB"
_FOOTER_SPEC = "<H"


class ArgSpecJson(TypedDict):
    name: str
    type: str
    format: Optional[str]


class LogMessageSpecJson(TypedDict):
    id: int
    message_template: str
    arguments: list[ArgSpecJson]


def _type_struct_fmt(typename: str):
    fmts = {
        "uint8": "B",
        "uint16": "<H",
        "uint32": "<I",
        "uint64": "<Q",
        "int8": "b",
        "int16": "<h",
        "int32": "<i",
        "int64": "<q",
        "float32": "<f",
        "float64": "<d",
    }

    if typename not in fmts:
        raise RuntimeError(f"Invalid typename '{typename}'")

    return fmts[typename]


def _fmt_time(timestamp_ms: int) -> str:
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


def _fmt_log_level(log_level: int) -> str:
    lvl_name = {10: "TRACE", 20: "DEBUG", 30: "INFO", 40: "WARN", 50: "ERROR", 60: "FATAL"}.get(log_level)

    return f"{lvl_name:<5}"


def _fmt_log_line(*, log_level: int, timestamp: int, module: str, msg: str):
    return (
        f"{_color(log_level)}{_fmt_time(timestamp)} [{_fmt_log_level(log_level)}] {module}:  "
        f"{msg}{colorama.Style.RESET_ALL}"
    )


def _color(log_level: int) -> str:
    return {
        10: colorama.Fore.BLUE + colorama.Style.DIM,  # TRACE
        20: colorama.Fore.BLUE,  # DEBUG
        30: colorama.Fore.WHITE,  # INFO
        40: colorama.Fore.YELLOW,  # WARN
        50: colorama.Fore.RED,  # ERROR
        60: colorama.Fore.RED + colorama.Style.BRIGHT,  # FATAL
    }.get(log_level)


def _get_module_id(data: bytes):
    _, _, module_id, _, _ = struct.unpack(_HEADER_SPEC, data[: struct.calcsize(_HEADER_SPEC)])
    return module_id


class LogModuleSpecJson(TypedDict):
    id: int
    name: str
    messages: list[LogMessageSpecJson]


class LogMessage:
    def __init__(self, json_spec: LogMessageSpecJson):
        self._create_fmt_string(json_spec)
        self._args = [(arg["name"], _type_struct_fmt(arg["type"])) for arg in json_spec["arguments"]]

    def decode(self, payload: bytes):
        arg_values = dict()

        for arg_name, arg_fmt in self._args:
            arg_values[arg_name] = struct.unpack(arg_fmt, payload[: struct.calcsize(arg_fmt)])[0]
            payload = payload[struct.calcsize(arg_fmt) :]

        return self._template.format_map(arg_values)

    def _create_fmt_string(self, json_spec: LogMessageSpecJson):
        result = json_spec["message_template"]

        for arg in json_spec["arguments"]:
            if arg["format"] is not None:
                result = result.replace(f"{arg['name']}:{arg['format']}", arg["name"])

        self._template = result


class LogModule:
    def __init__(self, json_spec):
        self._id = json_spec["id"]
        self._name = json_spec["name"]

        self._messages = {msg_json["id"]: LogMessage(msg_json) for msg_json in json_spec["messages"]}

    def decode(self, raw_message: bytes):
        # Read header and footer.
        timestamp, log_level, module_id, message_id, actual_payload_len = struct.unpack(
            _HEADER_SPEC, raw_message[: struct.calcsize(_HEADER_SPEC)]
        )
        (crc,) = struct.unpack(_FOOTER_SPEC, raw_message[-struct.calcsize(_FOOTER_SPEC) :])

        # Calculate actual payload length, compare to received payload length.
        actual_payload_len = len(raw_message) - struct.calcsize(_HEADER_SPEC) - struct.calcsize(_FOOTER_SPEC)

        if actual_payload_len != actual_payload_len:
            raise RuntimeError(f"Invalid payload length: {actual_payload_len} != {actual_payload_len}")

        # Validate that the message is known.
        if message_id not in self._messages:
            raise RuntimeError(f"Invalid message id: {message_id}")

        # Decode the payload.
        payload = raw_message[struct.calcsize(_HEADER_SPEC) : -struct.calcsize(_FOOTER_SPEC)]
        msg = self._messages[message_id].decode(payload)

        # Create the log line.
        return _fmt_log_line(log_level=log_level, timestamp=timestamp, module=self._name, msg=msg)


def main():
    parser = argparse.ArgumentParser(description="Read and log binary log messages")
    parser.add_argument("--log-spec", type=pathlib.Path, help="Path to the JSON log specification file", required=True)
    parser.add_argument("--port", type=str, help="Serial port to read from", required=True)
    args = parser.parse_args()

    with args.log_spec.open("r") as fin:
        json_spec = json.load(fin)

    modules = {json_module["id"]: LogModule(json_module) for json_module in json_spec["modules"]}

    with serial.Serial(args.port, timeout=None) as ser:
        print(_fmt_log_line(log_level=20, timestamp=0, module="LogViewer", msg=f"Logging messages received on port {args.port}"))
        print(_fmt_log_line(log_level=20, timestamp=0, module="LogViewer", msg=f"Parsing messages based on {args.log_spec}"))

        while True:
            start_byte = ser.read(1)
            if start_byte == b"L":
                raw_message = ser.read(ser.in_waiting)

                try:
                    _module_id = _get_module_id(raw_message)
                    if _module_id not in modules:
                        raise RuntimeError(f"Unknown module id: {_module_id}")
                    print(modules[_module_id].decode(raw_message))
                except Exception as e:
                    print(f"{_color(60)}{_fmt_time(0)} [FATAL] Invalid message: {e}{colorama.Style.RESET_ALL}")


if __name__ == "__main__":
    sys.exit(main())
