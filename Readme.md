# 🌱 Smart GSM-Based Precision Irrigation Controller  
### *LDAM Mini-Project — Arduino + SIM800C + Soil Moisture Automation*  

## 👨‍💻 Authors  
- **Prakash Swami**  
- **Piyush Kanakdande**  
- **Darsh Sonsale**  
- **Akshita Verma**

---

## 📌 Project Overview  
Farmers often waste water and time by manually switching irrigation pumps.  
To solve this, we built a **GSM-controlled Smart Irrigation System** capable of:

- 📞 Remote pump control via **phone calls**
- 💬 Sending **SMS acknowledgments**
- 🌡 Automatic irrigation using a **soil moisture sensor**

This makes irrigation **smarter, faster, and more efficient**, reducing water wastage and improving crop health.

---

## 🚀 Features  

### 🔹 1. Automatic Moisture-Based Irrigation
- Pump starts automatically when soil is **DRY**
- Pump stops automatically when soil is **WET**
- Uses averaged sensor readings for accuracy

### 🔹 2. Manual GSM Control (via Phone Call)
Control cycle:
1. **1st Call → MANUAL ON** (Pump forced ON)  
2. **2nd Call → MANUAL OFF** (Pump forced OFF)  
3. **3rd Call → AUTO MODE** (Sensor-based control)

### 🔹 3. SMS Acknowledgment
After each call command, the user receives a confirmation SMS.

### 🔹 4. Safety Features
Even in Manual ON mode, the pump turns OFF automatically if the soil becomes too wet.

---

## 🛠 Components Used  

| Component              | Purpose                     |
|------------------------|-----------------------------|
| Arduino UNO            | Main microcontroller        |
| SIM800C GSM Module     | Handles calls & SMS         |
| Relay Module           | Controls water pump         |
| Soil Moisture Sensor   | Detects DRY/WET soil        |
| 5V / 10V Power Adapter | Power supply                |
| Diodes & Capacitors    | Circuit protection          |
| Submersible Water Pump | Irrigation pump             |

---

## 🔄 System Workflow  

### 📞 Remote Control  
1. User sends call to GSM module  
2. GSM detects the call  
3. Arduino processes the command  
4. Relay activates pump ON/OFF  
5. Another call toggles the next mode  

### 🌧 Auto Mode  
- Reads moisture every few seconds  
- DRY → Pump ON  
- WET → Pump OFF  

---

## 🔌 Circuit Diagram  

**Connections Summary:**
- SIM800C TX → Arduino Pin **2**  
- SIM800C RX → Arduino Pin **3**  
- Relay IN → Arduino Pin **7**  
- Moisture Sensor OUT → **A0**  
- Pump connected to relay NO + COM  
- GSM must have stable power supply (recommended separate 5V–12V)

---


---

## 🧠 Code Overview  

### ✔ GSM Initialization  
- AT test  
- Disable echo  
- Enable caller ID  
- Set SMS mode  
- Check network registration  
- Read signal strength  

### ✔ Moisture Monitoring  
- Takes 10 readings → averages them  
- Categorizes soil as **DRY / MODERATE / WET**

### ✔ Pump Control Engine  
Based on:
- Current mode  
- Soil condition  

Relay is **ACTIVE LOW** →  
- LOW = Pump ON  
- HIGH = Pump OFF  

---

## 📞 Mode Switching Logic  

| Call Number | Mode       | Description        |
|------------|-----------|--------------------|
| 1          | MANUAL ON  | Pump forced ON     |
| 2          | MANUAL OFF | Pump forced OFF    |
| 3          | AUTO MODE  | Sensor controls pump |

Each mode change triggers an SMS reply.

---

## 🧪 Testing Procedure  
1. Power GSM module for 2–3 seconds  
2. Wait for “GSM Ready” on Serial Monitor  
3. Call the SIM card number  
4. Observe relay & pump response  
5. Check SMS acknowledgment  

---

## 🛤 Future Scope  
- Add IoT dashboard + mobile app  
- Waterproof industrial enclosure  
- AI-based irrigation prediction  
- Multi-field management  
- Add temperature, humidity sensors  

---

## ✅ Conclusion  
- Farmers can remotely operate water pumps via GSM  
- Soil moisture sensor provides smart Auto Mode  
- System reduces water wastage  
- Reliable, low-cost, and easy to install  
- A practical and effective solution for real-world irrigation management  


## 📂 Project Structure  

