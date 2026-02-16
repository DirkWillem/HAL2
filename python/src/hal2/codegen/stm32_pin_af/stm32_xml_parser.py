import dataclasses
import pathlib
import re

import xml.etree.ElementTree as et

import hal2.codegen.stm32_pin_af.pin as pin


def parse_pin_xml_file(xml_file_path: pathlib.Path) -> list[pin.Pin]:
    """
    Parses a pin XML file as provided in the ST pin data repository.

    Args:
        xml_file_path: Path to the XML file to parse.

    Returns:
        List of all pins, with parsed information.
    """

    pin_re = re.compile(r"^P(?P<port>[A-Z])(?P<num>[0-9]+)")
    af_re = re.compile(r"^GPIO_AF(?P<num>[0-9]+)_[A-Za-z0-9]+$")

    # Locate the GPIO IP XML file
    gpio_ip_path = _find_gpio_ip_path(xml_file_path)
    root = et.parse(gpio_ip_path).getroot()

    pins: list[pin.Pin] = []

    # Iterate over all pin nodes
    gpio_pins = root.findall(".//{*}GPIO_Pin")
    for pin_node in gpio_pins:
        # Parse pin name to port and number
        pin_name = pin_node.attrib["Name"]

        if (m := pin_re.match(pin_name)) is None:
            continue

        port, num = m.group("port"), int(m.group("num"))

        # Determine what alternate functions the pin has
        afs: dict[str, int] = dict()

        pin_signal_nodes = pin_node.findall(".//{*}PinSignal")
        for pin_signal_node in pin_signal_nodes:
            possible_value_node = pin_signal_node.find(".//{*}SpecificParameter[@Name='GPIO_AF']/{*}PossibleValue")
            if possible_value_node is None:
                continue

            if (af_match := af_re.match(possible_value_node.text)) is None:
                continue

            af_num = int(af_match.group("num"))
            af_name = pin_signal_node.attrib["Name"]

            afs[af_name] = af_num

        pins.append(pin.Pin(port=port, num=num, afs=afs))

    return pins


def _find_gpio_ip_path(xml_file_path: pathlib.Path) -> pathlib.Path:
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
