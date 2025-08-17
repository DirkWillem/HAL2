import hal2.sil.app as sil_app
import hal2.sil.helpers.uart_modbus_client as modbus_client


if __name__ == "__main__":
    lib_path = "/Users/dirkvanwijk/Developer/ethermal/etherwall_firmware/build/cmake-build-debug/src/app/mostcal/libmostcal_sil.dylib"

    data = bytes([0x01, 0x03, 0x01, 0x00, 0x00, 0x01, 0x85, 0xF6])

    with sil_app.SilApp(lib_path) as app:
        client = modbus_client.UartModbusClient(app.get_uart("ext_uart"))
        client.write_register(0x0100, 1, device_id=2)
        print(client.read_holding_registers(0x0100, count=1, device_id=1))
