# Serial Port Protocol Project 🛰️  
**RCOM | Project 1**

This project implements a serial link-layer communication protocol between two virtual machines. It also includes a virtual cable simulator (`cable`) capable of introducing noise and disconnections to test the robustness and reliability of the implemented protocol.

---

## 📁 Project Structure

```
E:.
├── .gitignore
├── Makefile
├── README.txt
├── penguin.gif              # Test file to be transmitted
│
├── bin/                     # Compiled binaries
│   └── .gitignore
│
├── cable/                   # Virtual cable simulator (DO NOT MODIFY)
│   └── cable.c
│
├── src/                     # Source code for the protocol implementation
│   ├── main.c
│   ├── serial_port.c
│   ├── alarm.c
│   ├── state_machine.c
│   ├── application_layer.c
│   └── link_layer.c
│
└── include/                 # Header files (interfaces)
    ├── serial_port.h
    ├── alarm.h
    ├── state_machine.h
    ├── application_layer.h
    └── link_layer.h
```

---

## ⚙️ Compilation

To build all binaries, run:
```bash
make
```
The compiled executables will be placed inside the `bin/` directory

To build only the virtual cable simulator:
```bash
make cable
```

To clean all compiled objects:
```bash
make clean
```
---

## 🚀 Execution Guide

### 1. Start the Virtual Cable 
The virtual cable application simulates two connected serial ports (`/dev/ttyS0` and `/dev/ttyUSB0`).

**Option 1 — Manual execution:**
```bash
sudo ./bin/cable_app
```

**Option 2 — Using Makefile:**
```bash
sudo make run_cable
```

> ⚠️ The socat package must be installed to create the virtual serial links.
> Install it with:  
> ```bash
> sudo apt install socat
> ```

---

### 2. Test the Protocol (No Noise)

**2.1. Start the Receiver**
```bash
# Manual
./bin/main /dev/ttyS0 9600 rx penguin-received.gif

# Using Makefile
make run_rx
```

**2.2. Start the Transmitter**
```bash
# Manual
./bin/main /dev/ttyS0 9600 tx penguin.gif

# Using Makefile
make run_tx
```

**2.3. Validate File Integrity**
```bash
# Manual
diff -s penguin.gif penguin-received.gif

# Using Makefile
make check_files
```
If both files match, the transmission was successful.

---

### 3. Test with Noise and Cable Disconnections
While the `cable` is running, you can interact with its console to simulate real-world link disturbances:

| Tecla | Ação                         |
|:-----:|------------------------------|
| 0     | Unplug the cable             |
| 1     | Restore normal operation     |
| 2     | Introduce transmission noise |

The link-layer protocol must handle retransmissions, timeouts, and frame corruption, ensuring that the received file remains identical to the original even under these conditions.

---

## 🧩 Implementation Notes

- All link-layer logic (framing, error detection, retransmission, and timeout control) should be implemented in `link_layer.c` and related modules.
- The `application_layer.c` handles high-level transmission and reception logic.
- The `state_machine.c` module supports frame parsing and validation.
- Use `make clean && make` after source modifications to ensure a proper rebuild.
- The `diff` command or `make check_files` verifies file integrity after transmission.
---

## 🧠 Credits
Academic project developed for RCOM course.
Base framework provided by the teaching staff.
Implementation and debugging by [@GoncaloMartins-exe](https://github.com/GoncaloMartins-exe) and [@rikivv](https://github.com/rikivv).

