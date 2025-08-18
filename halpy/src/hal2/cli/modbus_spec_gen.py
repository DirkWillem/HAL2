import dataclasses
import argparse
import pathlib

import hal2.modbus.generator.generator as generator


@dataclasses.dataclass
class Args:
    """Command-line arguments."""

    json_path: str
    """Path to the JSON file."""
    output_path: str
    """Path to the output file."""
    client_name: str
    """Name of the client to generate."""


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-i", "--json-path", required=True, help="Path to the JSON specification file.")
    parser.add_argument("-o", "--output-path", required=True, help="Path to the Python file to generate")
    parser.add_argument("-n", "--client-name", required=True, help="Name of the client to generate")

    args = parser.parse_args()

    generator.generate_python_from_json(pathlib.Path(args.json_path), pathlib.Path(args.output_path), args.client_name)


if __name__ == "__main__":
    main()
