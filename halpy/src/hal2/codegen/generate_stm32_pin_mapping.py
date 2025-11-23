import argparse
import pathlib
import sys

from hal2.codegen.stm32_pin_af.stm32_xml_parser import parse_pin_xml_file
from hal2.codegen.stm32_pin_af.periph_uart import gen_uart_mapping
from hal2.codegen.stm32_pin_af.periph_spi_i2s import gen_spi_i2s_mapping
from hal2.codegen.clang.clang_format import format_file


def main():
    parser = argparse.ArgumentParser(description="Generate stm32 pin mapping")
    parser.add_argument("--xml-file", type=pathlib.Path, help="XML file", required=True)
    parser.add_argument("--dst-file", type=pathlib.Path, help="Destination file", required=True)
    parser.add_argument("--mcu-family", type=str, help="MCU family", required=True)
    parser.add_argument("--mcu", type=str, help="Specific MCU", required=True)
    parser.add_argument(
        "--gen",
        type=str,
        choices=["uart", "spi_i2s"],
        help="What type of file to generate",
        required=True,
    )

    args = parser.parse_args()

    try:
        data = parse_pin_xml_file(args.xml_file)

        match args.gen:
            case "uart":
                args.dst_file.write_text(gen_uart_mapping(data, args.mcu_family, args.mcu))
            case "spi_i2s":
                args.dst_file.write_text(gen_spi_i2s_mapping(data, args.mcu_family, args.mcu))

        format_file(args.dst_file)
        return 0
    except Exception as e:
        print(e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
