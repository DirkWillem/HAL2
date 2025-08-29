import pathlib
import re

import xml.etree.ElementTree as et


def find_gpio_ip_path(xml_file_path: pathlib.Path) -> pathlib.Path:
    root = et.parse(xml_file_path).getroot()

    # Find GPIO IP node
    gpio_nodes = [node for node in root.findall(".//{*}IP") if node.attrib["Name"] == "GPIO"]
    if len(gpio_nodes) == 0:
        raise RuntimeError("Could not find IP node for GPIO")
    gpio_node = gpio_nodes[0]

    # Find version
    if "Version" not in gpio_node.attrib:
        raise RuntimeError("Could not find version attribute for GPIO IP node")
    version = gpio_node.attrib["Version"]

    # Find path
    gpio_ip_path = xml_file_path.parent / "IP" / f"GPIO-{version}_Modes.xml"
    if not gpio_ip_path.exists():
        raise RuntimeError(f"GPIO IP path {gpio_ip_path} does not exist")

    return gpio_ip_path


if __name__ == "__main__":
    xml_file_path = pathlib.Path(
        R"/Users/dirkvanwijk/Developer/ethermal/etherwall_firmware/src/hal/vendor/stm32_pin_data/mcu/STM32G030C(6-8)Tx.xml"
    )

    gpio_ip_path = find_gpio_ip_path(xml_file_path)
    print(gpio_ip_path)

    root = et.parse(gpio_ip_path).getroot()

    pin_re = re.compile(r"^P(?P<port>[A-Z])(?P<num>[0-9]+)")
    af_re = re.compile(r"^GPIO_AF(?P<num>[0-9]+)_[A-Za-z0-9]+$")

    gpio_pins = root.findall(".//{*}GPIO_Pin")
    for pin in gpio_pins:
        pin_name = pin.attrib["Name"]
        if (m := pin_re.match(pin_name)) is not None:
            port, num = m.group("port"), int(m.group("num"))
            print(port, num)

        print(et.tostring(pin).decode("utf-8"))
