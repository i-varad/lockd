# lockd

**A minimal X11 screen-locking experiment written in C.**

`lockd` is a Linux userspace project exploring X11 window management, keyboard and pointer control, event handling, rendering, and PAM-based user authentication.

The project is being developed from scratch to understand how a basic screen locker works internally.

## Current Progress

The current prototype implements:

- **X11 connection and window management** using Xlib.
- **Multi-monitor support** using Xinerama, with a fallback to the default X screen.
- **Lock-screen UI** rendered using Xft, including a clock, date, password mask, and authentication status.
- **X11 event handling** for keyboard input, window events, and UI redraws.
- **Keyboard and pointer grabs** to capture user input while the prototype is active.
- **Password entry** with masked input, Backspace handling, and Enter to submit.
- **PAM authentication** using Linux-PAM and the system's configured authentication and account policies.
- **Resource cleanup** for X11 windows, graphics contexts, fonts, and drawing resources.

## Technology Stack

- **Language:** C (C17)
- **Window system:** X11
- **Window and event handling:** Xlib
- **Text rendering:** Xft
- **Multi-monitor support:** Xinerama
- **Authentication:** Linux-PAM
- **Build system:** Make
- **Development environment:** Debian GNU/Linux

## Build

Install the required development libraries on Debian:

```bash
sudo apt install build-essential libx11-dev libxinerama-dev libxft-dev libpam0g-dev pkg-config
```

Clone the repository and build:

```bash
git clone https://github.com/i-varad/lockd
cd lockd
make
```

Run the prototype from your graphical X11 session:

```bash
./lockd
```

The project requires an X11 session and a configured PAM service named `lockd`. 

`/etc/pam.d/lockd` for the system-specific PAM service configuration used during development.
Will add this information soon once i polish things.

**Safety:** Keep a separate terminal or TTY available while testing. Do not run the prototype with elevated privileges.

## Current Authentication Flow

1. `lockd` connects to the X server.
2. It creates and maps a black window for each detected monitor.
3. It attempts to acquire keyboard and pointer grabs.
4. The user enters a password, which is displayed as masking characters.
5. Pressing Enter submits the username and password to the PAM authentication module.
6. Failed authentication leaves the prototype active for another attempt.
7. Successful authentication ends the prototype and releases the input grabs.

The current implementation accepts printable ASCII password input. Unicode input and more comprehensive failure handling remain future work.

## Upcoming Features

### 1. Lock Lifecycle and Failure-Handling Hardening

Make the lock lifecycle more robust before treating the program as a usable screen locker.

Planned work:

- Review keyboard and pointer grab acquisition and rollback.
- Prevent unauthenticated shutdown or window events from bypassing the lock.
- Define safe behavior for unexpected window destruction and X server errors.
- Review cleanup paths and input-grab release conditions.
- Test failure scenarios systematically.

### 2. Minimal (yet cool) Keyboard Input and Authentication UIUX

Improve password entry and make the authentication interface more complete.

Planned work:

- Support Unicode input using X11 input methods.
- Improve password-field behavior and input validation.
- Refine authentication status messages and redraw behavior.
- Test empty passwords, long input, Backspace, repeated failed attempts, and successful authentication.
- Add focused tests for password-input and authentication behavior.

## Project Status

**Experimental prototype — not production-ready.**

Although basic PAM authentication and X11 input grabs work, the project has not yet undergone a security audit. X11 session security, failure handling, and the reliability of the locking lifecycle still need careful review.

Do not rely on `lockd` to protect a real desktop session yet.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
