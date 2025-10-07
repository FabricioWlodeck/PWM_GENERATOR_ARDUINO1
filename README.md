# 🔧 PWM Soft-Start Generator (Arduino)

This project implements a **PWM signal generator** with a **linear and smooth duty cycle increase**, simulating a *soft-start* effect for DC motors or LEDs.  
The duty cycle ramps up gradually until reaching the programmed level — all done **purely in software**, without using hardware timers.

---

## ⚙️ Features
- Software-based PWM generation (no hardware timers used)  
- Linear duty cycle increase during startup  
- Adjustable soft-start duration  
- Visual LED feedback for duty cycle progression

---

## 🧩 Hardware
- **Microcontroller:** ATmega328P (Arduino Uno)  
- **Outputs:** PORTB pins driving progression LEDs and motor LED
---

## 🚀 How it works
1. The code generates a PWM cycle (100 Hz base)  
2. The duty cycle (`duty_time`) increases step by step  
3. LEDs indicate the current duty level (0–100%)

---

## 🧠 Example

- `duty_time < 20%` → All LEDs OFF  
- `20–40%` → 1 LED ON  
- `40–60%` → 2 LEDs ON  
- `60–80%` → 3 LEDs ON  
- `80–100%` → 4 LEDs ON  
- `>100%` → All LEDs ON (steady state)

---

## 🪄 Author
**Fabricio Wlodeck**  
💡 *Educational project for learning PWM and timing logic in embedded systems.*
