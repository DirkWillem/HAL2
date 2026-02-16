import pymodbus.client as modbus

import hal2.modbus.spec as spec


class Client:
    """
    MODBUS Client.
    """



    def __init__(self, c: modbus.ModbusBaseSyncClient, addr: int):
        """
        Constructor.

        Args:
            c: PyModbus client instance.
            addr: Slave address
        """
        self._c = c
        self._addr = addr

    def read_register[T](self, reg: spec.Register[T]) -> T:
        """
        Reads a register

        Args:
            reg: Register to read

        Returns:
            Read value
        """

        if reg.register_type == spec.RegisterType.INPUT_REGISTER:
            result = self._c.read_input_registers(reg.start_address, count=reg.size, device_id=self._addr)
        elif reg.register_type == spec.RegisterType.HOLDING_REGISTER:
            result = self._c.read_holding_registers(reg.start_address, count=reg.size, device_id=self._addr)
        else:
            raise RuntimeError(f"Invalid register type: {reg.type}")

        return self._decode_register_value(reg.type, result.registers)

    def write_register[T](self, reg: spec.Register[T], value: T):
        """
        Writes a holding register

        Args:
            reg: Register to write.
            value: Value to write.
        """

        # Validate we are writing to a holding register
        if reg.register_type == spec.RegisterType.INPUT_REGISTER:
            raise RuntimeError("Cannot write to an input register.")

        if isinstance(reg.type, spec.ScalarType):
            self._write_scalar(reg.start_address, value, reg.type)
        elif isinstance(reg.type, spec.EnumType):
            self._write_scalar(reg.start_address, value.value, reg.type.enum.get_underlying_type())
        elif isinstance(reg.type, spec.ArrayType):
            if isinstance(reg.type.element_type, spec.ScalarType):
                self._write_scalar(reg.start_address, value, reg.type.element_type)
            elif isinstance(reg.type.element_type, spec.EnumType):
                self._write_scalar(
                    reg.start_address, [v.value for v in value], reg.type.element_type.enum.get_underlying_type()
                )

    def _write_scalar(self, addr: int, value: int | list[int] | float | list[float], scalar_type: spec.ScalarType):
        self._c.write_registers(
            addr, values=self._c.convert_to_registers(value, self._get_scalar_type(scalar_type)), device_id=self._addr
        )

    def _decode_register_value(self, dt: spec.DataType, data: list[int]):
        if isinstance(dt, spec.ScalarType):
            return self._decode_scalar(data, dt)
        elif isinstance(dt, spec.EnumType):
            return dt.enum(self._decode_scalar(data, dt.enum.get_underlying_type()))
        elif isinstance(dt, spec.ArrayType):
            if isinstance(dt.element_type, spec.ScalarType):
                return self._decode_scalar(data, dt.element_type)
            elif isinstance(dt.element_type, spec.EnumType):
                return [
                    dt.element_type.enum(v)
                    for v in self._decode_scalar(data, dt.element_type.enum.get_underlying_type())
                ]

    def _decode_scalar(self, data: list[int], scalar_type: spec.ScalarType):
        return self._c.convert_from_registers(data, self._get_scalar_type(scalar_type))

    def _get_scalar_type(self, scalar_type: spec.ScalarType):
        scalar_types = {
            spec.ScalarType.U16: self._c.DATATYPE.UINT16,
            spec.ScalarType.U32: self._c.DATATYPE.UINT32,
            spec.ScalarType.U64: self._c.DATATYPE.UINT64,
            spec.ScalarType.I16: self._c.DATATYPE.INT16,
            spec.ScalarType.I32: self._c.DATATYPE.INT32,
            spec.ScalarType.I64: self._c.DATATYPE.INT64,
            spec.ScalarType.F32: self._c.DATATYPE.FLOAT32,
            spec.ScalarType.F64: self._c.DATATYPE.FLOAT64,
        }

        # Validate integer type
        if scalar_type not in scalar_types:
            raise ValueError(f"Invalid scalar type: {scalar_type}")

        return scalar_types[scalar_type]
