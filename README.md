# LayraPS4 Emulator
### A Modern, Standalone PlayStation 4 Emulator written in C

LayraPS4 is a next-generation open PS4 emulator focused on accurate emulation, modular subsystems, and a faithful user experience.  
Built from the ground up in **C** with a clean **ImGui-based PS4-style interface**, LayraPS4 aims to deliver stability, modularity, and authenticity.
<img width="1024" height="1536" alt="LayraPS4" src="https://github.com/user-attachments/assets/8b13cc98-3ad9-4dfa-9379-b78e70e2be0d" />

## 🔹 Project Overview

LayraPS4 is currently under **active development** as a complete reimplementation of the PS4 emulation environment — including package handling, virtual filesystem, and device simulation.  
It is **not** based on any existing emulator’s source code.  
Public open-source emulators (such as ShadPS4, RPCS3, or Kyty) are studied **only for structural reference and documentation purposes**, **never for direct code reuse**.

---

## 🔹 Core Goals

| Subsystem | Description | Status |
|------------|--------------|--------|
| **PKG Loader & Filesystem** | Support for `.pkg` game packages and virtual file system with `/app0/eboot.bin` mounting | ✅ Implemented |
| **VFS Architecture** | Internal structure using `MntPoints` and `HandleTable` for file and directory management | ✅ Implemented |
| **ImGui PS4 GUI** | Full PS4-style XMB interface with boot animation, themes, profiles, and system-like navigation | ⏳ In Progress |
| **Virtual Portals** | Simulation of **Skylanders**, **Disney Infinity**, and **LEGO Dimensions** portals and figures | 🚧 Developing |
| **Lightgun Support** | Emulation of lightgun devices and aiming calibration for supported titles | ⏳ Planned |
| **LAN/Local Multiplayer** | Basic LAN stack for local cooperative or VS play over emulated network | ⏳ Planned |
| **PSVR Integration** | Emulation layer for PSVR headset and motion devices | ⏳ Future Phase |

---

## 🔹 Design Principles

- **Accuracy through Documentation**  
  All implementations are based on documented behavior, packet studies, and hardware research — not copied code.

- **ImGui User Interface**  
  The UI replicates the native PS4 interface with animated boot-up, profiles, themes, and console-style navigation.

- **Cross-Platform Design**  
  Developed primarily for Windows, with Linux compatibility planned later.

- **Full Transparency**  
  All subsystems (PKG loader, crypto layer, VFS, portals, etc.) are documented in `/docs/`.

---

## 🔹 Project Structure

LayraPS4/
├── src/
│ ├── core/ # Emulator core systems (CPU, memory, scheduler)
│ ├── pkg/ # PKG parsing, decryption, and mounting
│ ├── vfs/ # Virtual filesystem and mount manager
│ ├── portals/ # Virtual portals subsystem
│ │ ├── portal_base.c
│ │ ├── portal_skylanders.c
│ │ ├── portal_infinity.c
│ │ ├── portal_legodimensions.c
│ │ └── figures/ # Virtual figure data and character files
│ ├── ui/ # ImGui PS4-style interface
│ └── utils/ # Helpers and shared modules
├── docs/
│ ├── architecture.md
│ ├── pkg_format.md
│ ├── vfs_structure.md
│ ├── portals_protocols.md
│ └── roadmap.md
├── CMakeLists.txt
└── README.md

yaml
Copy code

---

## 🔹 Virtual Portal Subsystem (Phase 2)

LayraPS4 will include virtual emulation of the following devices:

| Device | Functionality | Notes |
|---------|----------------|-------|
| **Skylanders Portal of Power** | Character data loading, LED control, figure scanning | Uses simple USB-like protocol |
| **Disney Infinity Base** | Character slots and playset base emulation | Custom packet protocol documented in `/docs/portals_protocols.md` |
| **LEGO Dimensions Toy Pad** | LED color management, NFC-based toy ID handling | Supports 3 pad slots with RGB mapping |

Each portal is handled through its own module with shared interface logic defined in `portal_base.c`.

🔹 Legal Notice

LayraPS4 is an independent reimplementation project and is not affiliated with Sony Interactive Entertainment.
This project does not include any copyrighted firmware, game content, or proprietary system modules.
Users must provide their own legally obtained PS4 firmware and game files for use.

Any study of other emulator repositories (e.g., RPCS3, ShadPS4) is strictly for structural and behavioral documentation purposes only — no source code is reused.

🔹 Future Roadmap

Finalize PKG/VFS subsystems ✅

Implement portal emulation (Skylanders, Infinity, LEGO Dimensions) 🚧

Build full ImGui PS4 GUI with system shell and themes ⏳

Integrate LAN support and controller mapping ⏳

Begin PSVR subsystem research ⏳

🔹 Credits

Lead Developer: DiverseBloom780
System Engineer: Manus (Autonomous Assistant Agent)
Acknowledgments:

Open-source emulator communities for technical references.

RPCS3, ShadPS4, and Kyty developers for their public research work.

Contributors maintaining documentation on PS4 system architecture.

© 2025 LayraPS4 Project. All rights reserved.

