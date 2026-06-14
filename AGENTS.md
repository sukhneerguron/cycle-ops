# AGENTS.md

# Project Philosophy

This project is a professional embedded firmware project targeting an ESP32-S3 using the ESP-IDF framework and FreeRTOS.

The project prioritizes:

- Maintainability
- Testability
- Separation of concerns
- Deterministic behavior
- Modular architecture
- Long-term extensibility

The project is NOT optimized for minimum code size or shortest implementation.

Favor clarity over cleverness.

---

# Technology Stack

Platform:
- ESP32-S3
- ESP-IDF
- FreeRTOS

Libraries:
- LVGL
- NimBLE
- Wire
- SPI

Language:
- Modern C++

---

# Core Architecture

The firmware follows a layered architecture.

```
                Consumers
        (Display / BLE / Logger)

                     ▲

                  Models

                     ▲

                 Services

                     ▲

                  Drivers

                     ▲

              ESP32 Hardware
```

Dependencies always flow downward.

Consumers never call Drivers.

Drivers never know Consumers exist.

---

# Ownership Rule

Every object in the system has exactly one owner.

Examples:

ADC peripheral
    owned by ADCDriver

Power calculations
    owned by PowerService

Power state
    owned by PowerModel

BLE characteristics
    owned by BLEConsumer

LVGL widgets
    owned by DisplayConsumer

No object may have multiple writers.

---

# Single Writer Multiple Reader

Every Model has exactly one writer.

Multiple readers are allowed.

Example:

PowerModel

Writer:
    PowerService

Readers:
    BLEConsumer
    DisplayConsumer
    LoggerConsumer

No other object may modify PowerModel.

---

# Drivers

Drivers own hardware.

Examples:

ADCDriver

GPIODriver

DisplayDriver

BLEDriver

TouchDriver

HallDriver

BatteryDriver

Drivers perform:

- peripheral initialization
- peripheral configuration
- interrupt handling
- DMA
- timestamping

Drivers never implement business logic.

Drivers never know bicycle concepts.

---

# Interrupt Philosophy

Interrupt handlers must execute as quickly as possible.

Allowed:

- timestamp
- push to queue
- notify task

Forbidden:

- floating point math
- printf
- display updates
- BLE operations
- filesystem access
- dynamic allocation

ISR execution should be measured in microseconds.

---

# Services

Services own business logic.

Examples:

PowerService

CadenceService

RideService

ECGService

TemperatureCompensationService

PowerFilteringService

DebounceService

Services transform inputs into Models.

Services do not access display objects.

Services do not access BLE objects.

Services should be portable C++ and preferably executable on a desktop PC.

---

# Models

Models own application state.

Models contain state only.

Models should contain little or no algorithmic logic.

Examples:

RideModel

PowerModel

ECGModel

BatteryModel

BLEModel

DisplayModel

---

# Consumers

Consumers present Models externally.

Examples:

DisplayConsumer

BLEConsumer

LoggerConsumer

StorageConsumer

Consumers read Models.

Consumers never calculate business values.

Consumers never own hardware.

---

# Communication

Subsystems communicate through:

- Queues
- Event Groups
- Task Notifications
- Immutable Model Snapshots

Subsystems should not directly call unrelated subsystems.

Example:

GOOD

ADCDriver

↓

Queue

↓

PowerService

↓

PowerModel

↓

DisplayConsumer


BAD

ADCDriver

↓

Display

---

# RTOS Rules

Use FreeRTOS primitives.

Preferred:

Tasks

Queues

Mutexes

Semaphores

Event Groups

Task Notifications

Avoid polling.

Tasks should normally block while idle.

Example:

for(;;)
{
    wait

    process

    publish

    sleep
}

---

# Queue Philosophy

High-frequency data:

ISR

↓

Queue

↓

Service

Examples:

wheel ticks

cadence ticks

touch events

ADC samples

---

# Event Philosophy

Low-frequency state changes:

RideStarted

RideStopped

BLEConnected

BLEDisconnected

CalibrationStarted

BatteryLow

Use Event Groups or Event Bus.

---

# Model Philosophy

Models are immutable to readers.

Services publish new snapshots.

Readers should never require long mutex locks.

Prefer:

copy

↓

atomic swap

↓

multiple readers

instead of large shared mutable structures.

---

# Display Rules

Display owns LVGL.

Display reads Models.

Display never computes values.

Display never touches GPIO.

Display never accesses ADC.

Display should be replaceable without modifying Services.

---

# BLE Rules

BLE owns BLE stack.

BLE reads Models.

BLE never calculates cadence.

BLE never calculates power.

BLE simply exposes Models using BLE services.

Supported services may include:

CSC

Cycling Power

Heart Rate

Battery

Device Information

---

# Power Rules

PowerService owns:

ADC processing

zero offset

temperature compensation

digital filtering

torque

power

balance

PowerService publishes PowerModel.

---

# ECG Rules

ECGService owns:

sampling

filtering

peak detection

RR interval

heart rate

ECGService publishes ECGModel.

---

# File Structure

src/

drivers/

services/

models/

consumers/

application/

events/

common/

tests/

Within each layer folder, group related files by feature into subdirectories (e.g., `consumers/ble/`, `services/cadence/`).

No feature should become a dumping ground.

Small focused modules are preferred.

---

# Memory

Prefer static allocation.

Avoid heap allocation after initialization.

Never allocate memory inside ISRs.

PSRAM is reserved for:

LVGL

framebuffers

fonts

large buffers

history

---

# Logging

Every module shall define its own TAG.

Use levels:

ERROR

WARN

INFO

DEBUG

No printf().

---

# Code Style

Modern C++

constexpr preferred

enum class preferred

RAII preferred

Avoid macros.

Avoid hidden globals.

Prefer explicit dependencies.

---

# Testability

Business logic should compile without ESP32 hardware.

Filtering

Temperature compensation

Power calculations

ECG processing

Cadence calculations

should all be executable and unit testable on a desktop PC.

---

# AI Coding Rules

Before generating code:

Identify:

Driver

Service

Model

Consumer

Generate code for only one responsibility.

Never combine responsibilities.

If uncertain, create another Service instead of increasing coupling.

Favor modularity over fewer files.

Favor explicit ownership over convenience.

Every generated file should have a single clearly defined responsibility.