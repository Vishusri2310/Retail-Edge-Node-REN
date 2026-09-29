# 🏪 REN: Edge-Native Intelligent Retail Analytics

**Decentralized AI Appliance for Privacy-Preserving Shopper & Inventory Dynamics**

[![Smart India Hackathon 2026](https://img.shields.io/badge/SIH_2026-Retail_Analytics-orange.svg)](#)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](#)
[![Python 3.11](https://img.shields.io/badge/python-3.11-blue.svg)](#)
[![TensorRT](https://img.shields.io/badge/TensorRT-8.6-76B900?logo=nvidia&logoColor=white)](#)

> **Team:** Runtime Rebels
> **Problem Statement:** Intelligent Retail Analytics System
> **Organization:** Smart India Hackathon 2026

---

## 📖 Project Overview
Physical retail suffers from a critical visibility gap regarding shopper behavior, stock-outs, and queue congestion, especially in Tier-2 and Tier-3 cities with constrained internet. **REN** solves this by transforming existing store CCTV infrastructure into a real-time spatial analytics engine. By executing hardware-accelerated AI models strictly on local edge devices, REN provides low-latency operational intelligence without requiring continuous cloud connectivity or compromising customer privacy.

## ✨ Key Innovations
*   🧠 **Predictive Queue Intelligence:** Calculates aisle departure velocity vectors to anticipate queue congestion 3 to 5 minutes before it physically forms[cite: 1].
*   📍 **Spatial Shopper Analytics:** Uses vector cross-products and homography matrices to track footfall, measure dwell times, and generate 10cm x 10cm floor heatmaps[cite: 1].
*   📦 **Automated Inventory Monitoring:** Detects shelf voids and dead stock in real-time, cross-referencing planograms to trigger instant restocking alerts.
*   🔒 **Privacy by Design (Zero-PII):** Processes raw video exclusively in volatile RAM and permanently purges it in under 33 milliseconds[cite: 1]. Zero biometric data is stored or transmitted, ensuring total DPDPA compliance[cite: 1].

---

## 🏗️ System Architecture

Our solution utilizes a decentralized, three-node hardware pipeline to eliminate single points of failure:

1. **Data Acquisition:** Ingests RTSP video streams from existing store cameras via zero-copy Video4Linux2 (V4L2) DMA buffers[cite: 1].
2. **Node A (Vision Core):** An NVIDIA Jetson Orin NX executes INT8 YOLOv8-Nano inference[cite: 1]. It converts video into abstract spatial coordinates, scrubs the RAM instantly, and handles all spatial vector geometry[cite: 1].
3. **Node B (Inventory Hub):** A local POS/Server manages planogram logic, identifies stock-outs from designated optical transducers, and integrates with legacy ERP systems (Tally, Marg).
4. **Harmonization & Telemetry:** Node A and Node B package metrics into lightweight JSON payloads (~15 KB/s) and broadcast them via a local Eclipse Mosquitto MQTT broker[cite: 1].
5. **Node C (Analytics Dashboard):** Ingests the MQTT telemetry into DuckDB for historical KPI storage and state management.
6. **Delivery:** A React/Three.js frontend serves a real-time digital twin, heatmaps, and actionable alerts to store supervisors on the local network.

---

## 🚀 Core Modules & Mathematical Logic

### 1. Shopper Analytics & Behavior
Replaces inaccurate IR break-beams and manual clipboards with precision spatial geometry[cite: 1].
*   **Directional Tripwires:** Evaluates virtual doorway crossings using vector cross-products[cite: 1]. 
    $\text{Direction} = \text{sign}((B_x - A_x)(c_{yt} - A_y) - (B_y - A_y)(c_{xt} - A_x))$
*   **Micro-Zone Dwell Verification:** Uses ray-casting point-in-polygon checks paired with velocity gating ($\vert{}\vert{}\mathbf{v}_{avg}\vert{}\vert{} < 0.2 \text{ m/s}$) to separate genuine browsing from aisle transit[cite: 1].
*   **Homography Heatmaps:** Transforms camera pixel coordinates into accurate 10cm x 10cm store CAD floor coordinates without storing imagery[cite: 1].
    $[X_{floor}, Y_{floor}, 1]^T \sim H_{3\times3} * [c_x, c_y, 1]^T$

### 2. Queue Intelligence & Surge Prediction
Replaces reactive checkout monitoring with proactive counter-balancing[cite: 1].
*   **Predictive Arrival Vectors:** Projects customer departure velocities moving outward from shopping aisles toward the cash-wraps to predict oncoming crowd density[cite: 1].
    $\lambda_{predicted} = \sum_j \frac{\mathbf{v}_j \cdot \mathbf{u}_{checkout}}{\vert{}\vert{}\mathbf{d}_j\vert{}\vert{}}$
*   **Dynamic Counter Alerts:** Automatically triggers a local relay beacon to recommend opening additional billing counters 3 to 5 minutes before bottlenecks materialize[cite: 1].

### 3. Inventory Monitoring
*   **Stock-Out Detection:** Identifies stock-outs, shelf gaps, and dead stock earlier using dedicated shelf-facing optical transducers[cite: 2].
*   **Automated Alerts:** Cross-references visual voids with Node B planogram databases to push real-time restocking notifications to floor staff.

---

## 🔒 Privacy by Design (Zero-PII Pipeline)
REN is engineered to fundamentally eliminate data privacy liabilities[cite: 2].
*   **Volatile RAM Execution:** Raw video frames live exclusively in volatile LPDDR5 system RAM ring buffers[cite: 1].
*   **Sub-33ms Destruction:** Frames are permanently purged in $<33\text{ ms}$ immediately following centroid extraction[cite: 1]. 
*   **No Biometrics:** The pipeline assigns purely numerical tracking tokens via Kalman filters (ByteTrack), completely devoid of facial recognition or demographic classifiers[cite: 1]. Zero facial embeddings or identifiable imagery are stored on disk or transmitted over the network[cite: 1].

---

## 📈 Impact & Business Benefits

*   **Better Customer Experience:** Faster checkout and reduced queue congestion[cite: 2].
*   **Smarter Inventory:** Identify stock-outs, shelf gaps, and dead stock earlier[cite: 2].
*   **Efficient Workforce:** Real-time alerts enable better staff allocation[cite: 2].
*   **Actionable Retail Insights:** Use footfall, heatmaps, and dwell time to support better decisions[cite: 2].
*   **Increased Revenue Capture:** Anticipating checkout surges actively prevents walk-outs and reduces shopping cart abandonment.
*   **Lower Operating Costs:** No recurring cloud SaaS fees[cite: 2]. 
*   **Uninterrupted Operations:** Functions flawlessly during local internet outages without relying on continuous cloud connectivity.
*   **Existing Hardware Integration:** Repurposes a store's standard RTSP/ONVIF CCTV cameras and integrates with legacy ERP systems, eliminating expensive IT overhauls.
*   **Regulatory Liability Shield:** Zero-PII architecture with local data processing guarantees total DPDPA compliance and shields retailers from regulatory fines[cite: 2].

---

## 💻 Tech Stack

### Edge AI & Vision Core (Node A)
*   **Languages:** C++17, C11
*   **Deep Learning:** TensorRT 8.6, YOLOv8-Nano (INT8 Pruned), ByteTrack
*   **Computer Vision & Math:** OpenCV (CUDA/GStreamer), Eigen3

### Backend & Orchestration (Node B & C)
*   **Languages:** Python 3.11
*   **Frameworks:** FastAPI, `pyodbc` (ERP Integration)
*   **Data & Messaging:** SQLite, DuckDB, Eclipse Mosquitto MQTT

### Frontend Delivery
*   **Dashboard:** React.js, Three.js (Digital Twin)
*   **Deployment:** Docker, Yocto Embedded Linux

---

## 🚀 Getting Started

### Prerequisites
* NVIDIA Jetson Orin NX (Ubuntu 22.04 LTS)
* C++17 Compiler, CMake 3.16+
* OpenCV, Eigen3, libgpiod, Paho MQTT C++

### Installation

1. **Install System Dependencies**
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake libopencv-dev libgpiod-dev nlohmann-json3-dev libpaho-mqttpp-dev libpaho-mqtt-dev

2. **Clone the repository**
   ```bash
   git clone [https://github.com/RuntimeRebels/REN_VisionCore.git](https://github.com/RuntimeRebels/REN_VisionCore.git)
   cd REN_VisionCore

3. **Build the Core**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
