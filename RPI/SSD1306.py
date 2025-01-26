
import time

try:
    from typing import Optional, Type
    from types import TracebackType

    # Used only for type annotations.
    from busio import SPI
    from digitalio import DigitalInOut
except ImportError:
    pass



class SPIDevice:
    """
    Represents a single SPI device and manages locking the bus and the device
    address.

    :param ~busio.SPI spi: The SPI bus the device is on
    :param ~digitalio.DigitalInOut chip_select: The chip select pin object that implements the
        DigitalInOut API.
    :param bool cs_active_value: Set to True if your device requires CS to be active high.
        Defaults to False.
    :param int baudrate: The desired SCK clock rate in Hertz. The actual clock rate may be
        higher or lower due to the granularity of available clock settings (MCU dependent).
    :param int polarity: The base state of the SCK clock pin (0 or 1).
    :param int phase: The edge of the clock that data is captured. First (0) or second (1).
        Rising or falling depends on SCK clock polarity.
    :param int extra_clocks: The minimum number of clock cycles to cycle the bus after CS is high.
        (Used for SD cards.)

    .. note:: This class is **NOT** built into CircuitPython. See
      :ref:`here for install instructions <bus_device_installation>`.

    Example:

    .. code-block:: python

        import busio
        import digitalio
        from board import *
        from adafruit_bus_device.spi_device import SPIDevice

        with busio.SPI(SCK, MOSI, MISO) as spi_bus:
            cs = digitalio.DigitalInOut(D10)
            device = SPIDevice(spi_bus, cs)
            bytes_read = bytearray(4)
            # The object assigned to spi in the with statements below
            # is the original spi_bus object. We are using the busio.SPI
            # operations busio.SPI.readinto() and busio.SPI.write().
            with device as spi:
                spi.readinto(bytes_read)
            # A second transaction
            with device as spi:
                spi.write(bytes_read)
    """

    def __init__(
        self,
        spi: SPI,
        chip_select: Optional[DigitalInOut] = None,
        *,
        cs_active_value: bool = False,
        baudrate: int = 100000,
        polarity: int = 0,
        phase: int = 0,
        extra_clocks: int = 0
    ) -> None:
        self.spi = spi
        self.baudrate = baudrate
        self.polarity = polarity
        self.phase = phase
        self.extra_clocks = extra_clocks
        self.chip_select = chip_select
        self.cs_active_value = cs_active_value
        if self.chip_select:
            self.chip_select.switch_to_output(value=not self.cs_active_value)

    def __enter__(self) -> SPI:
        while not self.spi.try_lock():
            time.sleep(0)
        self.spi.configure(
            baudrate=self.baudrate, polarity=self.polarity, phase=self.phase
        )
        if self.chip_select:
            self.chip_select.value = self.cs_active_value
        return self.spi

    def __exit__(
        self,
        exc_type: Optional[Type[type]],
        exc_val: Optional[BaseException],
        exc_tb: Optional[TracebackType],
    ) -> bool:
        if self.chip_select:
            self.chip_select.value = not self.cs_active_value
        if self.extra_clocks > 0:
            buf = bytearray(1)
            buf[0] = 0xFF
            clocks = self.extra_clocks // 8
            if self.extra_clocks % 8 != 0:
                clocks += 1
            for _ in range(clocks):
                self.spi.write(buf)
        self.spi.unlock()
        return False


from micropython import const
import adafruit_framebuf as framebuf


try:
    # Used only for typing
    from typing import Optional
    import busio
    import digitalio
except ImportError:
    pass

_FRAMEBUF_FORMAT = framebuf.MVLSB



SET_CONTRAST = const(0x81)
SET_ENTIRE_ON = const(0xA4)
SET_NORM_INV = const(0xA6)
SET_DISP = const(0xAE)
SET_MEM_ADDR = const(0x20)
SET_COL_ADDR = const(0x21)
SET_PAGE_ADDR = const(0x22)
SET_DISP_START_LINE = const(0x40)
SET_SEG_REMAP = const(0xA0)
SET_MUX_RATIO = const(0xA8)
SET_IREF_SELECT = const(0xAD)
SET_COM_OUT_DIR = const(0xC0)
SET_DISP_OFFSET = const(0xD3)
SET_COM_PIN_CFG = const(0xDA)
SET_DISP_CLK_DIV = const(0xD5)
SET_PRECHARGE = const(0xD9)
SET_VCOM_DESEL = const(0xDB)
SET_CHARGE_PUMP = const(0x8D)


class _SSD1306(framebuf.FrameBuffer):
    """Base class for SSD1306 display driver"""

    # pylint: disable-msg=too-many-arguments
    def __init__(
        self,
        buffer: memoryview,
        width: int,
        height: int,
        *,
        external_vcc: bool,
        reset: Optional[digitalio.DigitalInOut],
        page_addressing: bool
    ):
        super().__init__(buffer, width, height, _FRAMEBUF_FORMAT)
        self.width = width
        self.height = height
        self.external_vcc = external_vcc
        # reset may be None if not needed
        self.reset_pin = reset
        self.page_addressing = page_addressing
        if self.reset_pin:
            self.reset_pin.switch_to_output(value=0)
        self.pages = self.height // 8
        # Note the subclass must initialize self.framebuf to a framebuffer.
        # This is necessary because the underlying data buffer is different
        # between I2C and SPI implementations (I2C needs an extra byte).
        self._power = False
        # Parameters for efficient Page Addressing Mode (typical of U8Glib libraries)
        # Important as not all screens appear to support Horizontal Addressing Mode
        if self.page_addressing:
            self.pagebuffer = bytearray(width + 1)  # type: Optional[bytearray]
            self.pagebuffer[0] = 0x40  # Set first byte of data buffer to Co=0, D/C=1
            self.page_column_start = bytearray(2)  # type: Optional[bytearray]
            self.page_column_start[0] = self.width % 32
            self.page_column_start[1] = 0x10 + self.width // 32
        else:
            self.pagebuffer = None
            self.page_column_start = None
        # Let's get moving!
        self.poweron()
        self.init_display()

    @property
    def power(self) -> bool:
        """True if the display is currently powered on, otherwise False"""
        return self._power

    def init_display(self) -> None:
        """Base class to initialize display"""

        for cmd in (
            SET_DISP | 0x00,
            SET_MEM_ADDR,
            0x00,  # horizontal mode
            SET_DISP_START_LINE | 0x00,
            SET_SEG_REMAP | 0x00,  # 0xA0
            SET_MUX_RATIO,
            self.height - 1,
            SET_COM_OUT_DIR | 0x00,  # 0xC0
            SET_DISP_OFFSET,
            0x00,
            SET_COM_PIN_CFG,
            0x12,  # or 0x02 for some 128x64 boards
            SET_DISP_CLK_DIV,
            0x80,  # normal clock
            SET_PRECHARGE,
            0xF1,  # or 0x22
            SET_VCOM_DESEL,
            0x40,  # or 0x30
            SET_CONTRAST,
            0xCF,  # not max
            SET_ENTIRE_ON,
            SET_NORM_INV,
            SET_CHARGE_PUMP,
            0x14,  # internal charge pump on
            SET_DISP | 0x01,
        ):
            self.write_cmd(cmd)
        self.fill(0)
        self.show()

    def poweroff(self) -> None:
        """Turn off the display (nothing visible)"""
        self.write_cmd(SET_DISP)
        self._power = False

    def contrast(self, contrast: int) -> None:
        """Adjust the contrast"""
        self.write_cmd(SET_CONTRAST)
        self.write_cmd(contrast)

    def invert(self, invert: bool) -> None:
        """Invert all pixels on the display"""
        self.write_cmd(SET_NORM_INV | (invert & 1))

    def rotate(self, rotate: bool) -> None:
        """Rotate the display 0 or 180 degrees"""
        self.write_cmd(SET_COM_OUT_DIR | ((rotate & 1) << 3))
        self.write_cmd(SET_SEG_REMAP | (rotate & 1))
        # com output (vertical mirror) is changed immediately
        # you need to call show() for the seg remap to be visible

    def write_framebuf(self) -> None:
        """Derived class must implement this"""
        raise NotImplementedError

    def write_cmd(self, cmd: int) -> None:
        """Derived class must implement this"""
        raise NotImplementedError

    def poweron(self) -> None:
        "Reset device and turn on the display."
        if self.reset_pin:
            self.reset_pin.value = 1
            time.sleep(0.001)
            self.reset_pin.value = 0
            time.sleep(0.010)
            self.reset_pin.value = 1
            time.sleep(0.010)
        self.write_cmd(SET_DISP | 0x01)
        self._power = True

    def show(self) -> None:
        """Update the display"""
        if not self.page_addressing:
            xpos0 = 0
            xpos1 = self.width - 1
            if self.width != 128:
                # narrow displays use centered columns
                col_offset = (128 - self.width) // 2
                xpos0 += col_offset
                xpos1 += col_offset
            self.write_cmd(SET_COL_ADDR)
            self.write_cmd(xpos0)
            self.write_cmd(xpos1)
            self.write_cmd(SET_PAGE_ADDR)
            self.write_cmd(0)
            self.write_cmd(self.pages - 1)
        self.write_framebuf()



# pylint: disable-msg=too-many-arguments
class SSD1306_SPI(_SSD1306):
    """
    SPI class for SSD1306

    :param width: the width of the physical screen in pixels,
    :param height: the height of the physical screen in pixels,
    :param spi: the SPI peripheral to use,
    :param dc: the data/command pin to use (often labeled "D/C"),
    :param reset: the reset pin to use,
    :param cs: the chip-select pin to use (sometimes labeled "SS").
    """

    # pylint: disable=no-member
    # Disable should be reconsidered when refactor can be tested.
    def __init__(
        self,
        width: int,
        height: int,
        spi: busio.SPI,
        dc: digitalio.DigitalInOut,
        reset: Optional[digitalio.DigitalInOut],
        cs: digitalio.DigitalInOut,
        *,
        external_vcc: bool = False,
        baudrate: int = 8000000,
        polarity: int = 0,
        phase: int = 0,
        page_addressing: bool = False
    ):
        self.page_addressing = page_addressing
        if self.page_addressing:
            raise NotImplementedError(
                "Page addressing mode with SPI has not yet been implemented."
            )

        self.rate = 1_000_000
        dc.switch_to_output(value=0)
        self.spi_device = SPIDevice(
            spi, cs, baudrate=baudrate, polarity=polarity, phase=phase
        )
        self.dc_pin = dc
        self.buffer = bytearray((height // 8) * width)
        super().__init__(
            memoryview(self.buffer),
            width,
            height,
            external_vcc=external_vcc,
            reset=reset,
            page_addressing=self.page_addressing,
        )

    def write_cmd(self, cmd: int) -> None:
        """Send a command to the SPI device"""
        self.dc_pin.value = 0
        with self.spi_device as spi:
            spi.write(bytearray([cmd]))

    def write_framebuf(self) -> None:
        """write to the frame buffer via SPI"""
        self.dc_pin.value = 1
        with self.spi_device as spi:
            spi.write(self.buffer)



