# 🐚 Multi-User Communicating Shells with Shared Messaging

## 📌 Project Description

This project is a GUI-based multi-user shell environment, where each terminal window acts like a UNIX shell and supports real-time messaging with other shell instances via a shared memory buffer.

Shells are implemented using **GTK** for the GUI, and they are capable of both executing typical shell commands and broadcasting messages across all instances.

---

## 🎯 Project Objectives

- Understand **process management** using `fork()` / `execvp()`.
- Explore **shared memory communication** and **semaphores**.
- Work with **GUI programming** using GTK.
- Apply **MVC architecture** to structure the project.
- Enable **real-time inter-shell communication**.

---

## 🧠 System Architecture (MVC)

### Model (`model.c`)

- Manages shell processes, command execution, redirection, and message buffer.
- Uses `fork()` to create child processes and `execvp()` to run commands.
- Implements shared memory communication via `shm_open` and `mmap`.
- Synchronizes access using `sem_t`.

### View (`view.c`)

- Provides GTK GUI: includes input (`GtkEntry`) and output (`GtkTextView`) widgets.
- Contains shell windows and message panels.
- Displays command results and incoming messages.

### Controller (`controller.c`)

- Parses input from the View.
- Decides whether input is a command or a message (`@msg Hello`).
- Calls `model_execute_command()` or `model_send_message()`.
- Polls shared memory regularly to fetch messages.

---

## 💻 How It Works

1. User types a command or message.
2. Controller checks:
    - If command: pass to Model → fork child → execute with `execvp()`.
    - If message: write to shared memory (`shmp->msgbuf`).
3. Output or messages are printed to the GUI using GTK widgets.

---

## 📁 File Structure

```bash
project/
├── controller.c    # Parses input and connects model-view
├── model.c         # Backend logic for processes and message handling
├── view.c          # GTK interface implementation
├── Makefile        # Compilation script
├── README.md       # Project documentation
└── report.pdf      # Project report with design details
```

---

## 🛠️ Installation & Usage

### Prerequisites

- Linux OS
- `gcc`, `make`
- GTK 3 (`libgtk-3-dev`)
- POSIX support (for `shm_open`, `sem_t`)

---

## 👥 Contributors

- [Amine Sayed](https://github.com/Amine86s)
- [Gülseli Ocakcı](https://github.com/gulseliocakci)

---

### Build the Project

```bash
make