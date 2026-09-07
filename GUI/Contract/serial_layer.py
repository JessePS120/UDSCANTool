import os
import serial
import serial.tools.list_ports
from Contract import macro

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_UART_HEADER = os.path.join(_REPO_ROOT, "App", "Inc", "uart.h")

class SerialLayer:
    #TODO: Need some platform specific code here for the serial port.
    #Pulled directly from App/Inc/uart.h so the two stay in sync.
    BAUD_RATE = int(macro.read_c_macro(_UART_HEADER, "UART_BAUD_RATE"))

    def __init__(self) -> None:
        self._ser: serial.Serial | None = None

    @staticmethod
    def get_com_list() -> list[str]:
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    @property
    def is_connected(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def connect(self, port: str) -> None:
        #Only one connection at a time for now.
        self.close()
        self._ser = serial.Serial(port, self.BAUD_RATE, timeout=1)

    def close(self) -> None:
        if self._ser is not None:
            self._ser.close()
            self._ser = None

    def send(self, data: bytes) -> None:
        if not self.is_connected:
            raise RuntimeError("Not connected to a serial port.")
        self._ser.write(data)

    def read_line(self) -> str:
        if not self.is_connected:
            raise RuntimeError("Not connected to a serial port.")
        line = self._ser.readline()
        return line.decode('utf-8').strip()
