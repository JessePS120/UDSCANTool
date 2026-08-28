# UDS-over-CAN Device — Project Requirements (v0.1) 

## 1. Project Goal

Build a standalone microcontroller that can transmit and receive CAN
messages implementing the UDS (ISO 14229-1) diagnostic protocol over
ISO-TP (ISO 15765-2) transport, acting initially as a **UDS tester (client)**.
The device should be able to send diagnostic requests, correctly
reassemble/segment multi-frame messages, and decode responses into a
human-readable form over a debug/host interface.

## 2. Scope
### Phase 1
- Using classic CAN 2.0B only. No CAN-FD. 
- Low amount of CAN/hardware error handling. Assumes no errors with CAN bus. 
- Provide some amount of hardware abstract so that code may be portable. 
- Using only physical addressing. No broadcast addressing. 
- Implement ISO-TP framing(as used by UDS). 
- Implement a small set of UDS services(to be defined later). 
- Talk to a second board using a UDS server stub OR a laptop emulating a server. 
- Create a UART to USB interface to allow a user to issue commands from the client board. NOTE: May need to be upgraded to USB if UART is not fast enough. 

### Phase 2 
- Using physical addressing **AND** broadcast addressing. 
- Client CAN customization at build time **AND** using the UART/USB interface. 
- Verify on real vehicle OR on laptop emulating a server. NOTE: Vehicle testing will include read services ONLY. 
- Basic logging features as part of UART to USB interface. 

### Phase 3 - Stretch Goals 
- Allow the client to choose between CAN classic and CAN-FD. 
- PC GUI built to give more insights into CAN data. 
- Expansion of available UDS commands. 

## 3. Functional Requirements 
### 3.1 CAN Link Layer 
- R-1: Client shall transmit and recieve 11-bit CAN 2.0B frames(phase 1) or 29 bit CAN 2.0B frames(phase 2). 
- R-2: Client shall be able to switch CAN baud rate at build time and via the terminal(phase 2). 
- R-3: Receiving CAN messages **MUST** be interrupt driven to avoid polling as CAN messages would easily overwhelm the CPU(phase 1). 
- R-4: Client shall use a device filter to filter out undesirable CAN messages(phase 1) configurable at build time or via the UART/USB interface(phase 2). 

### 3.2 ISO-TP Transport Layer
- R-5: Client shall segment outgoing UDS messages larger than 7 bytes into First Frame + Consecutive Frame(s) then reassemble incoming multi-frame messages the same way(phase 1).
- R-6: Client shall support Single Frame for messages ≤7 bytes(phase 1).
- R-7: Client shall implement Flow Control frames and transmit FC after receiving a First Frame obey received FC (block size, STmin) when sending Consecutive Frames(phase 1).
- R-8: Client shall implement transport-layer timeouts per spec: N_As, N_Ar, N_Bs, N_Cr (default 1000 ms each per ISO 15765-2)(phase 1), and abort and report a transport error on timeout rather than hanging(phase 2).
- R-9: Client shall allow physical (1-to-1) addressing(phase 1) and (1-to-many) addressing(phase 2).

### 3.3 UDS Application Layer(Work In Progress). 

### 3.4 UART/USB Interface 
- R-10: Client shall expose a UART to USB interface that: 
    1. Allows the host to issue support UDS requests with user provided parameters(phase 1). 
    2. View the response from UDS requests(phase 1) and log the data(phase 2). 
    3. Customize the client(Work In Progress)(phase 2 and 3): 
        1. Swtich between 11-bit and 29bit CAN frames(phase 2). 
        2. Create a CAN message filter(phase 2). 
        3. Switch between CAN classic and CAN FD(phase 3). 
    4. Has a corresponding host GUI that allows the user to issue commands and view data(phase 3). 

### 3.5 Hardware Specific 
- R-11: All client code shall include wrappers around HAL code provided STM32 Cube IDE. 

### 3.6 Testing and Validation Plan(Work In Progress). 

### 3.7 Hardware 
- Nucleo-G474RE (lacks CAN transceiver). 
- SN65HVD230 breakout board (CAN transciever). 
- Twisted pair wires. 
- Two 120ohm termination resistors. 
- CANable(for testing). 
