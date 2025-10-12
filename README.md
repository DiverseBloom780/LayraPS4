# LayraPS4
 is a standalone PS4 emulator written in C, using the general architecture and logic patterns studied from ShadPS4 and other open-source PS4 emulators.
The emulator will feature:

Fully reimplemented PKG support (no extraction required).

PSVR headset and PSVR Aim gun emulation.

Lightgun input support for compatible titles.

Virtual portal systems for:

Disney Infinity

Skylanders

LEGO Dimensions

Replacement of Qt GUI with ImGui for modern, lightweight rendering and configuration.

A modular structure ready for future LAN/local multiplayer emulation.

Entirely self-contained — no external frontends or launcher integration.

🔹 Core Features & Goals
1. PKG Integration

Integrate PKG support from v0.7.0-era source logic into the modern system architecture.

Re-implement .pkg mounting logic directly inside the emulator core.

.pkg files should mount into the virtual filesystem as /app0/, with eboot.bin accessible without extraction.

Require decrypted PKG for execution.

Use 11.0 firmware module structure for compatibility.

2. PSVR & Lightgun System

Implement emulation of the PSVR headset (motion tracking, video feed, and sensor detection).

Add PSVR Aim gun emulation — connect through virtual Bluetooth like the controller system.

Add general lightgun support for non-VR shooting titles, recognized as controller extensions.

3. Virtual Portals and Characters

Study the portal and figure logic from RPCS3 (only study behavior and communication, never code).

Document command IDs, message flow, and data structures for:

Disney Infinity Base

Skylanders Portal

LEGO Dimensions Toy Pad

Re-implement those devices in C as virtual USB/Bluetooth peripherals.

Implement figure data loading, activation, and communication using the studied behavior patterns.

4. GUI and Interface (ImGui)

Fully transition from Qt to ImGui.

Build a clean, minimal interface with:

Game list

Settings/config window

Logging console

Device connection status (VR, controllers, portals)

Optimize for Windows first (future Linux/Mac ports possible).

5. LAN/Local Network (Planned)

Plan and design for future LAN emulation layer.

Allow local multiplayer or multi-system LAN linking (e.g., LEGO Dimensions cooperative play).

Not for online connectivity — LAN-only framework.

🔹 Research and Legal Boundaries
Approved for Study:

ShadPS4 (latest build) – for architectural design and current compatibility logic.

Kyty, FpPS4, Orbital – for reference on early structure and low-level boot/kernel logic.

Excluded from Study:

Spine (Linux-only; not applicable to this base).

Psoff (closed source).

Any code under GPLv2 or non-permissive license.

Study Rules:

Observe and document, never copy.

All reimplementation must be original and written in C.

Maintain clear internal documentation of:

Structures and functions

Behavior flow and device protocols

File mounting and VFS logic

🔹 System Architecture Notes

Firmware base: 11.0 modules.

Core designed for modular device support:

Controllers

PSVR headset

Lightguns

Portals

Maintain isolation between:

Loader (PKG/VFS subsystem)

Emulator core (CPU/GPU/SPU)

Device emulation (VR, portals, input)

Interface (ImGui layer) 
<img width="1024" height="1536" alt="LayraPS4" src="https://github.com/user-attachments/assets/8b13cc98-3ad9-4dfa-9379-b78e70e2be0d" />
