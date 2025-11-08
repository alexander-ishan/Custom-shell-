# 🧠 Advanced UNIX Shell Implementation in C++

A feature-rich custom **UNIX shell** built entirely in **C++**, implementing core functionalities of popular shells like `bash` and `zsh`.  
This project demonstrates **process management, I/O redirection, piping, background execution, and environment variable handling** using **low-level POSIX system calls**.

---

## 🚀 Features

✅ **Command Execution** – Run standard Linux commands  
✅ **Input/Output Redirection** – Supports `>`, `>>`, `<`, `2>`, `2>>`, and `&>`  
✅ **Piping** – Chain multiple commands with `|`  
✅ **Background Jobs** – Execute processes using `&`  
✅ **Environment Variables** – Use and export variables (`export VAR=value`)  
✅ **Built-in Commands** – `cd`, `exit`, `export`, `jobs`  
✅ **Error Handling** – Robust checks for invalid commands and missing files  

---

## 🧩 Technologies Used

- **C++17**
- **POSIX System Calls:** `fork()`, `execvp()`, `pipe()`, `dup2()`, `waitpid()`, `chdir()`, etc.
- **UNIX/Linux environment**
- **Makefile** (optional, for compilation automation)

---

## ⚙️ Installation & Setup

### 1️⃣ Clone the Repository
```bash
git clone https://github.com/<your-username>/Advanced-Unix-Shell.git
cd Advanced-Unix-Shell
```
### 2️⃣ Compile the Program
```bash
g++ myshell.cpp -o mysh
```
###3️⃣ Run the Shell
```bash
./mysh
```
📁 Project Structure
```bash
├── shell.cpp          # Main source code for shell
├── README.md          # Project documentation
├── Makefile           # Optional build file
└── out.txt            # Example output file (created at runtime)
```
📜 License

This project is open-source and available under the MIT License.

👨‍💻 Author

Alexander Ishan
🔗 GitHub: @alexander-ishan

📧 Email: ishannasker18@gmail.com
