from hal2.logging.spec import IntFormatter


def test_formatter_int_default():
    fmt = IntFormatter(None)

    assert fmt.format(1) == "1"
    assert fmt.format(1234) == "1234"


def test_formatter_int_bases():
    bin_fmt = IntFormatter("b")

    assert bin_fmt.format(1) == "1"
    assert bin_fmt.format(10) == "1010"

    hex_fmt = IntFormatter("x")
    assert hex_fmt.format(10) == "a"
    assert hex_fmt.format(0x1234ABCD) == "1234abcd"

    hex_fmt = IntFormatter("X")
    assert hex_fmt.format(10) == "A"
    assert hex_fmt.format(0x1234ABCD) == "1234ABCD"


def test_formatter_int_padding():
    fmt = IntFormatter("<4")

    assert fmt.format(1) == "1   "
    assert fmt.format(123) == "123 "
    assert fmt.format(1234) == "1234"
    assert fmt.format(12345) == "12345"

    fmt = IntFormatter(">4")

    assert fmt.format(1) == "   1"
    assert fmt.format(123) == " 123"
    assert fmt.format(1234) == "1234"
    assert fmt.format(12345) == "12345"

    fmt = IntFormatter("04")

    assert fmt.format(1) == "0001"
    assert fmt.format(123) == "0123"
    assert fmt.format(1234) == "1234"
    assert fmt.format(12345) == "12345"

    fmt = IntFormatter("08x")

    assert fmt.format(0xABC) == "00000abc"
