Sure! Here's the updated **README** section with the required installation instructions for **CURL** and **POSIX threads** on both **Linux** and **Windows**:

---

## 📥 Multi-threaded Download Manager

This is a **C-based** multi-threaded download manager that allows fast and efficient downloading of files from direct URLs using multiple threads.

### 🛠️ Technologies Used

* **C Language**: Core implementation language for system-level performance and memory control.
* **libCURL**: Used for handling the downloading of files over various protocols (HTTP, HTTPS, FTP) via direct URLs.
* **POSIX Threads (Pthreads)**: Used to implement multi-threading, allowing concurrent downloading to increase speed and responsiveness.
* **Thread Synchronization**: Mutexes and other POSIX synchronization primitives are used to ensure thread-safe operations and avoid race conditions during download and file writing.

### ⚙️ Installation

Before running the code, make sure the required libraries are installed:

#### On **Linux**:

```bash
sudo apt update
sudo apt install libcurl4-openssl-dev
```

> POSIX threads are natively supported on Linux (via `-lpthread`), so no additional installation is needed.

#### On **Windows**:

1. **libCURL**:

   * Download pre-built CURL binaries from: [https://curl.se/windows/](https://curl.se/windows/)
   * Extract and link against the `libcurl.lib` or include the DLL during runtime.

2. **POSIX Threads** (via pthreads-win32):

   * Download from: [https://sourceware.org/pthreads-win32/](https://sourceware.org/pthreads-win32/)
   * Link against `pthreadVC2.lib` and include headers in your project setup.

> On Windows, make sure to configure your compiler (like MinGW or Visual Studio) to recognize the library paths.

### ✅ Features

* Supports **multi-threaded** downloads using direct URLs.
* Efficient and faster downloading through thread-based chunk management.
* Uses **libCURL** for robust and reliable downloading.
* Thread-safe implementation with **POSIX thread synchronization**.
* Lightweight and written entirely in **C** for portability and performance.
