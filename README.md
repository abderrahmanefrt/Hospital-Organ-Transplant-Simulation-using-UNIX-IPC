# 🏥 Hospital Organ Transplant Simulation (UNIX IPC)

## 📌 Overview
This project is a **multi-process simulation of a hospital organ transplant system**, implemented in **C** using **UNIX System V IPC mechanisms**.

The goal is to model how **critical and non-critical patients**, **surgeons**, and **donors** coordinate in a concurrent environment while respecting synchronization constraints.

This project was developed as part of an **Operating Systems / Concurrency** course.

---

## 🧠 System Architecture

The system consists of **four main processes**:

### 👨‍⚕️ 1. Critical Patients (`MaladeCr`)
- Send organ requests with **high priority**
- Receive organs via message queue
- Must be served before non-critical patients

### 🧍 2. Non-Critical Patients (`MaladeNCr`)
- Send organ requests with **lower priority**
- Wait for availability after critical patients

### 🩺 3. Surgeon (`Chirurgien`)
- Receives patient requests from message queues
- Transfers requests to donors via a pipe
- Retrieves fabricated organs from shared memory
- Sends results back to patients

### 🧬 4. Donor (`Donneur`)
- Produces organs based on received requests
- Deposits organs into a shared buffer (fridge)
- Respects buffer capacity using semaphores

---

## 🔧 IPC Mechanisms Used

| Mechanism | Purpose |
|---------|--------|
| **Message Queues** | Communication between patients and surgeon |
| **Shared Memory** | Shared organ buffer (fridge) |
| **Semaphores** | Synchronization (mutex + empty slots) |
| **Pipes** | Surgeon → Donor communication |
| **Fork** | Process creation |

---

## 🔐 Synchronization Model

- **Producer–Consumer problem**
- Shared circular buffer
- Semaphores:
  - `nvide` → number of empty slots
  - `mutex` → mutual exclusion
- Priority handling for critical patients

---

## 📦 Data Structures

- `StructureDemande` → organ request
- `StructureReponse` → transplant result
- `ElementTampon` → organ unit
- `MemoirePartagee` → circular shared buffer

---

## ▶️ Compilation & Execution

### Compile
```bash
gcc -Wall -o hospital hospital.c
