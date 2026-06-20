# UI Architecture

## Philosophy

The UI is a first-class subsystem.

The UI is responsible only for presenting application state and collecting user interaction.

The UI must never contain business logic.

The UI must never directly access:

- GPIO
- ADC
- BLE
- Filesystem
- Sensor drivers
- RTOS queues

The UI renders Models and generates UI events.

---

# Directory Structure

```
main/ui/

    components/

    screens/

    navigation/

    theme/

    assets/
```

---

# Components

Components are reusable visual elements.

Examples:

```
SpeedWidget

PowerWidget

CadenceWidget

HeartRateWidget

BatteryWidget

RideTimerWidget

StatusBar

ConnectionIndicator
```

Components should be small and focused.

Each component should ideally fit within a single source file pair.

```
SpeedWidget.cpp

SpeedWidget.hpp
```

Components own only their own LVGL objects.

---

# Screens

Screens compose components.

Examples:

```
RideScreen

PowerScreen

HealthScreen

SettingsScreen

DiagnosticsScreen
```

A screen is responsible for layout only.

Example:

```
RideScreen

├── SpeedWidget
├── PowerWidget
├── CadenceWidget
├── DistanceWidget
└── StatusBar
```

Screens should not perform calculations.

Screens should not query hardware.

Screens should simply arrange and update components.

---

# Navigation

Navigation owns screen transitions.

Only the Navigation subsystem may:

```
lv_scr_load()

lv_screen_load_anim()

change active screen
```

Components should never change screens directly.

Instead they publish navigation events.

---

# Theme

All colors, fonts and spacing must be centralized.

```
theme/

    Colors.hpp

    Fonts.hpp

    Metrics.hpp
```

Forbidden:

```
lv_obj_set_style_text_color(...hardcoded...)
```

Preferred:

```
Theme::PrimaryTextColor()

Theme::LargeNumberFont()
```

Changing themes should not require modifying widgets.

---

# Data Flow

Data flow is strictly one direction.

```
Driver

↓

Service

↓

Model

↓

UI

↓

LVGL
```

The UI is always the final consumer.

Components must never update Models.

Components never call Services.

Components never access Drivers.

---

# Widget Updates

Updates are Event-Driven using a Publish-Subscribe (Pub/Sub) pattern.

Widgets subscribe to specific messages when they are created. 
When a Service updates a Model, a message is published, and the widget's callback is triggered by LVGL to update its own objects.
LVGL 9 introduced `lv_subject_t` and `lv_observer_t` to natively replace `lv_msg`. This is a decoupled Pub/Sub system.

Because observer callbacks execute *immediately* when a subject is updated, **you must only update subjects while holding the LVGL lock**, otherwise the RTOS will crash. This is why DisplayTask exists, It is the only task allowed to update subjects.

```cpp
// 1. Define a shared subject (e.g., in a ModelStore)
lv_subject_t ride_subject;
lv_subject_init_pointer(&ride_subject, nullptr);

// 2. Widget subscribes during creation
lv_subject_add_observer_obj(&ride_subject, PowerWidget::onRideDataObserved, this->container, this);

// 3. DisplayTask wakes up and updates the subject
if (bits & RIDE_DATA_UPDATED) {
    driver_.lock(); // Lock LVGL
    lv_subject_set_pointer(&ride_subject, (void*)&RideService::getModel());
    driver_.unlock();
}

// 4. Widget callback automatically fires (safely under lock)
void PowerWidget::onRideDataObserved(lv_observer_t* observer, lv_subject_t* subject) {
    PowerWidget* widget = (PowerWidget*)lv_observer_get_user_data(observer);
    const RideModel* model = (const RideModel*)lv_subject_get_pointer(subject);
    
    if (model) {
        lv_label_set_text_fmt(widget->valueLabel, "%d W", model->power);
    }
}
```

### Critical Rules for Updates:

Widgets should never calculate:

- power
- cadence
- temperature compensation

Formatting for display is acceptable.

Business logic is forbidden.

---

# Component Responsibilities

GOOD

SpeedWidget

- display speed
- animate speed
- format speed text

BAD

SpeedWidget

- calculate speed
- read hall sensor
- access BLE
- access ADC

---

# Screen Responsibilities

GOOD

RideScreen

- create widgets
- position widgets
- update widgets

BAD

RideScreen

- calculate ride statistics
- read battery voltage
- access filesystem
- perform BLE operations

---

# LVGL Rules

LVGL object ownership must be explicit.

Every component owns the LVGL objects it creates.

Example:

```
class SpeedWidget
{
private:

    lv_obj_t* container;

    lv_obj_t* valueLabel;

    lv_obj_t* unitsLabel;
};
```

Global LVGL objects are discouraged.

---

# UI Refresh

Who triggers the widget updates?

This is the important part.

There must be exactly ONE UI task. This task runs the core LVGL engine, which natively processes all events, subscriptions, and timers.

Example:

```cpp
void DisplayTask(void* pvParameters)
{
    while(true)
    {
        lv_lock(); // Required if LVGL is used in an RTOS environment

        lv_timer_handler(); // Processes messages, timers, and redraws

        lv_unlock();

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}
```

Widgets DO NOT create their own FreeRTOS tasks. They rely entirely on `lv_timer_handler()` running in the main Display Task to fire their callbacks.

Data flow for an update:
```
Service (e.g. PowerService)
↓
Updates PowerModel
↓
Publishes "PowerUpdated" Event
↓
DisplayTask (lv_timer_handler)
↓
PowerWidget Callback Fired
```

This ensures thread safety because all LVGL rendering and label updates strictly occur within the single `DisplayTask`.

---

# Layout Philosophy

Prefer multiple simple screens over crowded dashboards.

Example:

Ride Screen

```
Speed

Power

Cadence

Distance
```

Power Screen

```
Power

Balance

Torque

Efficiency
```

Health Screen

```
Heart Rate

RR Interval

Battery

Temperature
```

Readability while riding is more important than displaying every available value.

---

# Component Design

Components should be:

- reusable
- self-contained
- independent
- easy to test

Components should expose a minimal interface.

Example:

```
create(parent)

show()

hide()
```

No component should know what screen it belongs to.

---

# Code Organization

Preferred:

```
ui/

    components/

        SpeedWidget/

            SpeedWidget.cpp

            SpeedWidget.hpp

        PowerWidget/

        BatteryWidget/

    screens/

        RideScreen/

            RideScreen.cpp

            RideScreen.hpp

        SettingsScreen/

    navigation/

    theme/

    assets/
```

Organize by feature, not by file type.

---

# Memory Placement

The ESP32-S3 has both fast internal SRAM and slower external PSRAM.

**Rule:** LVGL draw buffers MUST be allocated in internal SRAM.
DMA controllers require fast memory to push pixels to the display efficiently.

**Rule:** Large assets (custom fonts, background images) and widget allocations should be placed in PSRAM to conserve precious internal memory.

---

# Animations

For data that updates rapidly (e.g., Power or Cadence updating at 10Hz), do NOT use LVGL animations (`lv_anim_t`).

Attempting to animate rapidly changing values wastes CPU cycles and creates visual lag. Simply update the value directly.

Reserve smooth animations for screen transitions, popups, or low-frequency state changes.

---

# Design Philosophy

This project intentionally does NOT implement a React clone.

Instead it borrows the following ideas:

- component composition
- reusable widgets
- immutable inputs
- one-way data flow
- separation of presentation and business logic

while remaining idiomatic C++ and LVGL.