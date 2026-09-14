# UDSCANTool

A standalone UDS (ISO 14229-1) diagnostic tester built on an STM32 Nucleo-G474RE. The board
can generated UDS messages over ISO-TP (ISO 15765-2) on a classic CAN 2.0B bus, and is driven either from a
plain serial terminal or from the bundled Python GUI over the ST-Link virtual COM port.

---

## 1. Overview

### 1.1 What it does

The device acts as a **UDS client (tester)**. You issue a command from the host, the firmware
builds the UDS request, segments it through ISO-TP, puts it on the CAN bus, waits for the
server's response, reassembles it, and prints the decoded result back to the host.

Current capabilities:

- **Raw CAN** — send an arbitrary frame (up to 8 bytes) and stream received frames live.
- **UDS TesterPresent** (SID `0x3E`) — keep-alive / bus liveness check.
- **UDS VIN read** — ReadDataByIdentifier (SID `0x22`) against DID `0xF190`.
- **Negative response decoding** — NRCs are translated to readable text rather than raw bytes.

Addressing is physical 1-to-1: the tester transmits on **`0x7E0`** and accepts responses on
**`0x7E8`** (see [cli.h](App/cli/cli.h)). The bus runs at **500 kbit/s**, classic CAN, 11-bit
identifiers.

For the full requirement list see [Docs/PROJECTREQUIREMENTS.md](Docs/PROJECTREQUIREMENTS.md);
for requirement implementation status see [Docs/TRACEABILITY.md](Docs/TRACEABILITY.md).

### 1.2 Architecture

The firmware is layered so that everything above the HAL wrapper is portable C:

```
        Host (PC)
  +-----------------------+
  |  GUI/main.py          |  customtkinter GUI
  |  Contract/            |  pyserial wrapper + shared constants
  +-----------+-----------+
              |  USB  (ST-Link VCP, 115200 8N1)
  ============+====================================  Nucleo-G474RE
              |
  +-----------v-----------+
  |  App/cli              |  command parser / console loop
  +-----------------------+
  |  App/uds              |  UDS service layer (SIDs, NRCs, P2 timing)
  +-----------------------+
  |  App/isotp-c          |  ISO-TP segmentation (isotp-c + port layer)
  +-----------------------+
  |  App/can   App/uart   |  portable CAN / UART interfaces
  +-----------------------+
  |  Core/Src/*_hal_*.c   |  STM32G474RE HAL implementations
  +-----------+-----------+
              |  CAN TX/RX (PA12 / PA11)
  +-----------v-----------+
  |  SN65HVD230           |  CAN transceiver
  +-----------+-----------+
              |  CAN_H / CAN_L twisted pair, 120 ohm at each end
       ECU / CANable / second board
```

### 1.3 Repository layout

| Path | Contents |
| --- | --- |
| [App/cli/](App/cli/) | Console loop, command dispatch, peripheral bring-up order |
| [App/uds/](App/uds/) | UDS request building, response/NRC parsing, timing config |
| [App/isotp-c/](App/isotp-c/) | Vendored [isotp-c](https://github.com/lishen2/isotp-c) library plus the project's port layer (`isotp_port.c`, `isotp_user.c`) |
| [App/can/](App/can/) | Portable CAN interface (`CANInit`, `CANSend`, `CANReceive`) |
| [App/uart/](App/uart/) | Portable UART interface and the RX ring buffer |
| [Core/Src/](Core/Src/) | STM32G474RE HAL implementations of the above interfaces, plus CubeMX-generated startup code |
| [Utils/](Utils/) | Utility functions: `Status` codes, deferred logger, timer, LED, critical sections |
| [GUI/](GUI/) | Python GUI application |
| [Docs/](Docs/) | Requirements and traceability matrix |
| [Drivers/](Drivers/) | ST HAL and CMSIS (vendored, do not edit) |

### 1.4 Conventions

- **Error handling.** Every function that requires error handling returns a `Status` enum from
  [Utils/status.h](Utils/status.h) (`STATUS_OK`, `STATUS_TIMEOUT`, `STATUS_PROTOCOL_ERROR`, ...).
  Callers propagate it upward while the CLI is the only layer that turns a `Status` into text sent over 
  UART. 
- **Logging.** `LOG_ERROR` / `LOG_WARN` / `LOG_INFO` in [Utils/log.h](Utils/log.h) write into a
  ring buffer that is drained to UART outside of interrupt context by `logDrain()`. This keeps
  ISRs free of blocking I/O.
- **Hardware abstraction.** Nothing under `App/` includes an STM32 header. Porting to another
  MCU means reimplementing the `*_hal_*.c` files in `Core/Src/`.
- **Interrupt-driven RX.** CAN and UART reception both land in ring buffers from their ISRs and
  the application polls those buffers. 

### 1.5 Branches

Two branches: **`master`** and **`in-progress`**. Work lands on `in-progress` and is merged into
`master` once tested against [Docs/PROJECTREQUIREMENTS.md](Docs/PROJECTREQUIREMENTS.md).

---

## 2. Hardware

### 2.1 Bill of materials

| Item | Purpose | Notes |
| --- | --- | --- |
| ST Nucleo-G474RE | Main board (STM32G474RET6, 170 MHz) | Has an FDCAN controller but **no** on-board transceiver |
| SN65HVD230 breakout | CAN transceiver | 3.3 V logic, so it pairs directly with the G474 |
| 2 x 120 ohm resistors | Bus termination | One at each physical end of the bus |
| Twisted pair wire | CAN_H / CAN_L | Keep stubs short |
| CANable (or second Nucleo) | Counterpart under test | Acts as the UDS server / traffic source |
| USB Micro-B cable | For Power, flashing, and the serial console | The ST-Link VCP carries the CLI, so that no extra USB-UART adapter is needed |

### 2.2 Pin assignment

| Signal | Pin | Peripheral | Goes to |
| --- | --- | --- | --- |
| CAN RX | PA11 | FDCAN1_RX (AF9) | SN65HVD230 `RXD` |
| CAN TX | PA12 | FDCAN1_TX (AF9) | SN65HVD230 `TXD` |
| Console TX | PA2 | LPUART1_TX | ST-Link VCP |
| Console RX | PA3 | LPUART1_RX | ST-Link VCP |
| Status LED | PA5 | GPIO output | On-board LD2 |
| SWDIO / SWCLK | PA13 / PA14 | SWD | On-board ST-Link |

Transceiver power: `3V3` and `GND` from the Nucleo's Morpho/Arduino headers.

### 2.3 Bus timing

Configured in [can_hal_stm32g474re.c](Core/Src/can_hal_stm32g474re.c): FDCAN is clocked from
PCLK1 at 170 MHz, prescaler 10, giving a 34 tq bit time (`TimeSeg1 = 29`, `TimeSeg2 = 4`,
`SJW = 4`) for **500 kbit/s**. 

## 3. Building and running

### 3.1 Firmware

**Requirements:** [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
**1.19.0**. Newer versions should work but are untested. The Arm GNU toolchain and ST-Link
drivers ship with the IDE.

1. `File > Open Projects from File System...` and select the repository root. The project is
   recognized from `.project` / `.cproject`; do **not** use `Import > Existing Project` on a copy
   of the folder.
2. Connect the Nucleo over USB Micro-B (the `CN1` ST-Link connector).
3. `Project > Build Project` (Ctrl+B). Artifacts land in `Debug/` (git-ignored).
4. `Run > Debug As > STM32 C/C++ Application`, or click Run to flash and go.

Peripheral configuration lives in [UDSCANTool.ioc](UDSCANTool.ioc). Regenerating from CubeMX
rewrites `Core/Src/main.c` and `Core/Src/stm32g4xx_hal_msp.c`, so keep any custom code inside the
`USER CODE BEGIN/END` markers. Generally, it is not recommended to regenerate any code. 

Key compile-time macros:

| Macro | File | Default |
| --- | --- | --- |
| `UART_BAUD_RATE` | [App/uart/uart.h](App/uart/uart.h) | `115200` |
| `UARTRingBufMaxSize` | [App/uart/uart.h](App/uart/uart.h) | `150` |
| `CANRingBufMaxSize` | [App/can/can.h](App/can/can.h) | `16` |
| `CAN_CLIENT_TX_ID` / `CAN_SERVER_RX_ID` | [App/cli/cli.h](App/cli/cli.h) | `0x7E0` / `0x7E8` |
| `UDS_P2_CLIENT_MS` | [App/uds/uds_cfg.h](App/uds/uds_cfg.h) | `50` |
| `UDS_P2_STAR_CLIENT_MS` | [App/uds/uds_cfg.h](App/uds/uds_cfg.h) | `5000` |
| `MAX_UDS_MSG_SIZE` | [App/uds/uds_cfg.h](App/uds/uds_cfg.h) | `100` |
| `LOG_LEVEL` | [Utils/log.h](Utils/log.h) | `LOG_LEVEL_INFO` |

If you change `UART_BAUD_RATE` the GUI picks it up automatically because it parses the macro
straight out of the header (see 3.3).

### 3.2 Using the serial console

Any terminal works (PuTTY, `screen`, the CubeIDE terminal). Open the ST-Link virtual COM port at
**115200 8N1**. The firmware prints `Enter CMD` when it is ready for input.

Every command is `KEYWORD:ARGUMENT` and the colon is mandatory.

| Command | Effect |
| --- | --- |
| `UDS:TESTERPRESENT` | Send a TesterPresent request and print the response |
| `UDS:VIN` | Read DID `0xF190` and print the returned bytes |
| `CANS:<bytes>` | Transmit a raw frame on `0x7E0`, max 8 bytes |
| `CANR:` | Stream every received frame. Press any key to stop |

Example session:

```
Enter CMD
UDS:VIN
UDS response SID: 0x62 Containing: F1 90 31 47 31 ...
Enter CMD
```

A rejected request prints the decoded NRC for example `UDS rejected: Service not supported`.

### 3.3 Running the GUI

**Requirements:** Python 3.11 or newer (3.13 is what the project is developed using) and the
packages in [GUI/requirements.txt](GUI/requirements.txt): `customtkinter`, `pyserial`,
`darkdetect`, `packaging`.

```bash
cd GUI
python -m venv venv
.\venv\Scripts\Activate.ps1 # Windows
pip install -r requirements.txt
python main.py
```

Start `main.py` from inside `GUI/` so the `Contract` package resolves.
[Contract/serial_layer.py](GUI/Contract/serial_layer.py) locates
[App/uart/uart.h](App/uart/uart.h) relative to the repository root and reads `UART_BAUD_RATE` out
of it at import time. That is the "contract" idea described in
[GUI/Contract/README.md](GUI/Contract/README.md): the firmware headers are the single source of
truth and the host code derives its settings from them instead of duplicating constants.

Workflow in the app:

1. Pick the ST-Link COM port from the top dropdown (the list refreshes every second).
2. Click **Use COM Port**.
3. Choose a UDS command and click **Send UDS Command**, type hex into the CAN entry and send it,
   or click **Start Receiving CAN Messages** to stream traffic.

The GUI tracks board state and blocks a second command until the firmware has printed its
`Enter CMD` prompt again, so the device is never handed overlapping requests.

> Close the GUI before opening a terminal on the same port, and vice versa. Only one process can
> hold the COM port.

### 3.4 Screenshot

![GUI](Docs/Images/gui.png)

*The host application: COM port selection, UDS command menu, raw CAN entry, and the output
console.*

### 3.5 Tests

Testing is currently a work in progress. 

### 3.6 Documentation

API documentation is generated with Doxygen from [doxygen_config](doxygen_config):

```bash
doxygen doxygen_config
```

Output lands in `html/` and `latex/`, both git-ignored. Open `html/index.html` to browse.

---

## 4. TO-DO 
- Implement uds_client.c and uds_server.c in preparation of creating a test suite. 
- Create test cases using uds_client.c and uds_server.c. 
- Add more UDS commands to uds.c. 

## 4. Credits

ISO-TP segmentation is handled by [isotp-c](https://github.com/SimonCahill/isotp-c), vendored under
[App/isotp-c/](App/isotp-c/) and adapted to this project's `Status` and logging conventions
through `isotp_port.c` and `isotp_user.c`.
