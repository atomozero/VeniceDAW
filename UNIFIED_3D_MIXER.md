# Unified 3D Mixer Window

## 🎯 Overview

VeniceDAW now has a **unified 3D mixer window** that combines:
- **Classic 3dmix Mode**: Simple, beautiful 3D visualization (BeOS heritage)
- **Spatial Audio Mode**: Advanced 3D positioning with HRTF processing

**One window, two modes!** No more separate windows to manage.

## 🚀 Architecture

### Previous Design (Deprecated):
```
❌ Mixer3DWindow          (simple 3D)
❌ SpatialMixer3DWindow   (spatial audio)
   → Two separate windows, code duplication
```

### New Unified Design:
```
✅ Mixer3DWindow (unified)
   ├─ Mixer3DView        (simple mode - 3dmix heritage)
   └─ SpatialMixer3DView (spatial mode - extends Mixer3DView)
   → One window, dynamic mode switching
```

## 🔧 Usage

### Creating a Simple 3dmix Window:
```cpp
// Classic 3dmix mode (no spatial processing)
SimpleHaikuEngine* engine = GetAudioEngine();
Mixer3DWindow* mixer = new Mixer3DWindow(engine);
mixer->Show();
```

### Creating a Spatial Audio Window:
```cpp
// Advanced spatial mode (with HRTF)
SimpleHaikuEngine* engine = GetAudioEngine();
AdvancedAudioProcessor* processor = GetAudioProcessor();
Mixer3DWindow* mixer = new Mixer3DWindow(engine, processor);
mixer->Show();
// Automatically starts in spatial mode if processor provided
```

### Switching Modes at Runtime:
```cpp
// User clicks "🌐 Spatial Mode" button
mixer->SetSpatialMode(true);   // Switch to spatial

// User clicks "🎚️ Simple Mode" button
mixer->SetSpatialMode(false);  // Switch to classic 3dmix
```

## 🎨 Visual Design

### Simple Mode (3dmix heritage):
- ✨ Glossy spheres with glow halos
- 🌊 Glass floor with BeOS gradient
- 🎨 BeOS signature color palette (yellow, orange, cyan)
- 💫 Floating animation
- 📊 Vertical level indicators

### Spatial Mode (adds):
- 🎯 3D track positioning (drag & drop)
- 👂 Listener visualization (green head)
- 🔊 Speaker layout (5.1/7.1 surround)
- 🌐 Room boundaries wireframe
- 🎧 HRTF processing indicators

## 🎛️ Controls

### Common Controls (both modes):
- **▶ Play 3D** - Start audio playback
- **⏹ Stop** - Stop playback
- **📷 Reset** - Reset camera to default view
- Mouse drag - Rotate camera
- `+`/`-` keys - Zoom in/out
- `R` key - Reset camera

### Mode Switcher (if processor available):
- **🌐 Spatial Mode** - Switch to advanced spatial audio
- **🎚️ Simple Mode** - Switch to classic 3dmix

### Spatial Mode Only:
- Drag tracks - Position in 3D space
- Spatial controls panel (right side)
- HRTF configuration
- Room acoustics settings

## 📊 Implementation Details

### Polymorphic View System:
```cpp
class Mixer3DWindow {
    Mixer3DView* f3DView;  // Can be base or derived class

    // Runtime type checking
    SpatialMixer3DView* GetSpatialView() {
        return dynamic_cast<SpatialMixer3DView*>(f3DView);
    }
};
```

### Mode Switching Process:
1. Remove old view from window
2. Delete old view (cleanup OpenGL context)
3. Create new view (appropriate type)
4. Add new view to layout
5. Update UI labels and info text

### Memory Safety:
- ✅ Proper OpenGL context cleanup before deletion
- ✅ Smart pointer considerations (manual delete for now)
- ✅ Safe dynamic_cast with nullptr checks
- ✅ Thread-safe view updates

## 🎵 Features Comparison

| Feature | Simple Mode | Spatial Mode |
|---------|-------------|--------------|
| 3dmix spheres | ✅ Yes | ✅ Yes |
| Glass floor | ✅ Yes | ✅ Yes |
| Glow effects | ✅ Yes | ✅ Yes |
| BeOS colors | ✅ Yes | ✅ Yes |
| Camera controls | ✅ Yes | ✅ Yes |
| Track positioning | ❌ No | ✅ Yes |
| HRTF binaural | ❌ No | ✅ Yes |
| Surround sound | ❌ No | ✅ Yes |
| Room acoustics | ❌ No | ✅ Yes |
| Doppler effect | ❌ No | ✅ Yes |

## 🔍 Code Locations

### Core Files:
- `src/gui/Mixer3DWindow.h` - Unified window header
- `src/gui/Mixer3DWindow.cpp` - Window implementation with mode switching
- `src/gui/SpatialMixer3DWindow.h` - Spatial view extension
- `src/gui/SpatialMixer3DWindow.cpp` - Spatial features implementation

### Key Methods:
- `Mixer3DWindow::SetSpatialMode(bool)` - Toggle mode
- `Mixer3DWindow::SwitchToSpatialMode()` - Internal switch to spatial
- `Mixer3DWindow::SwitchToSimpleMode()` - Internal switch to simple
- `Mixer3DWindow::GetSpatialView()` - Safe cast to spatial view

## 🚧 Migration Guide

### For existing code using separate windows:

**Before:**
```cpp
// Old separate windows
Mixer3DWindow* simple = new Mixer3DWindow(engine);
SpatialMixer3DWindow* spatial = new SpatialMixer3DWindow(engine, processor);
```

**After:**
```cpp
// New unified window
Mixer3DWindow* mixer = new Mixer3DWindow(engine, processor);
// Automatically in spatial mode

// Or start in simple mode:
Mixer3DWindow* mixer = new Mixer3DWindow(engine);
mixer->Show();
```

### SpatialMixer3DWindow is now deprecated:
- ⚠️ Still exists for compatibility
- 🔄 Internally uses same classes
- 📌 Prefer using unified `Mixer3DWindow`

## ✨ Benefits of Unification

1. **Single Window Management** - User sees one "3D Mixer" window
2. **Seamless Switching** - Toggle modes without closing/reopening
3. **Code Reuse** - SpatialMixer3DView extends Mixer3DView
4. **Better UX** - Discover advanced features via button
5. **Less Memory** - Only one window, one OpenGL context
6. **Cleaner API** - One constructor, optional processor parameter

## 🎉 Result

Users now have a **unified, professional 3D mixer** that:
- Starts simple (classic 3dmix beauty)
- Scales to advanced (spatial audio power)
- Maintains BeOS heritage throughout
- Provides smooth, intuitive workflow

**One window. Two worlds. Infinite possibilities.** 🚀
