/*
 * KeyboardShortcuts.h - Centralized keyboard shortcuts system
 * Provides professional DAW-style keyboard shortcuts
 */

#ifndef KEYBOARD_SHORTCUTS_H
#define KEYBOARD_SHORTCUTS_H

#include <SupportDefs.h>

namespace HaikuDAW {

/*
 * Keyboard shortcut definitions inspired by Pro Tools, Logic Pro, and Ableton Live
 *
 * Categories:
 * - Transport: Play, Stop, Record, navigation
 * - Editing: Cut, Copy, Paste, Delete, Duplicate
 * - Navigation: Zoom, Pan, Fit
 * - Selection: Select All, Deselect
 * - Tracks: Add, Remove, Mute, Solo
 * - Views: Switch between Mixer, 3D, Timeline
 */

// Transport shortcuts
const char KEY_PLAY_PAUSE = ' ';        // Spacebar - toggle play/pause
const char KEY_STOP = '.';              // Period - stop playback
const char KEY_RECORD = '*';            // Asterisk - record
const char KEY_REWIND = ',';            // Comma - rewind
const char KEY_FORWARD = '/';           // Slash - fast forward
const char KEY_RETURN_TO_ZERO = B_HOME; // Home - return to start

// Editing shortcuts (with modifiers)
const char KEY_CUT = 'X';               // Cmd+X - cut
const char KEY_COPY = 'C';              // Cmd+C - copy
const char KEY_PASTE = 'V';             // Cmd+V - paste
const char KEY_DUPLICATE = 'D';         // Cmd+D - duplicate
const char KEY_SPLIT = 'E';             // Cmd+E - split at cursor
const char KEY_DELETE = B_DELETE;       // Delete key

// Navigation shortcuts
const char KEY_ZOOM_IN = '=';           // = (or +) - zoom in
const char KEY_ZOOM_OUT = '-';          // - - zoom out
const char KEY_FIT_TO_WINDOW = 'F';     // Cmd+F - fit all to window
const char KEY_SCROLL_LEFT = B_LEFT_ARROW;
const char KEY_SCROLL_RIGHT = B_RIGHT_ARROW;

// Selection shortcuts
const char KEY_SELECT_ALL = 'A';        // Cmd+A - select all tracks/clips
const char KEY_DESELECT_ALL = B_ESCAPE; // Esc - deselect all

// Track shortcuts
const char KEY_NEW_TRACK = 'T';         // Cmd+T - create new track
const char KEY_REMOVE_TRACK = 'R';      // Cmd+R - remove selected track
const char KEY_MUTE = 'M';              // M - toggle mute on selected
const char KEY_SOLO = 'S';              // S - toggle solo on selected
const char KEY_SELECT_NEXT_TRACK = B_DOWN_ARROW;   // Down - select next track
const char KEY_SELECT_PREV_TRACK = B_UP_ARROW;     // Up - select previous track

// View shortcuts
const char KEY_SHOW_MIXER = '1';        // Cmd+1 - show mixer window
const char KEY_SHOW_3D = '2';           // Cmd+2 - show 3D mixer
const char KEY_SHOW_TIMELINE = '3';     // Cmd+3 - show timeline (future)
const char KEY_SHOW_INSPECTOR = 'I';    // Cmd+I - toggle inspector panel

// Import/Export shortcuts
const char KEY_IMPORT_AUDIO = 'I';      // Cmd+I - import audio file
const char KEY_IMPORT_MULTIPLE = 'M';   // Cmd+Shift+M - import multiple
const char KEY_IMPORT_3DMIX = '3';      // Cmd+Shift+3 - import 3dmix project
const char KEY_EXPORT_AUDIO = 'E';      // Cmd+Shift+E - export audio

// Quick access shortcuts
const char KEY_UNDO = 'Z';              // Cmd+Z - undo
const char KEY_REDO = 'Y';              // Cmd+Y - redo
const char KEY_SAVE = 'S';              // Cmd+S - save project
const char KEY_SAVE_AS = 'S';           // Cmd+Shift+S - save as
const char KEY_QUIT = 'Q';              // Cmd+Q - quit application

/*
 * Modifier flags for keyboard shortcuts
 * Use these to check which modifiers are pressed
 */
const uint32 MOD_NONE = 0;
const uint32 MOD_SHIFT = B_SHIFT_KEY;
const uint32 MOD_CONTROL = B_CONTROL_KEY;
const uint32 MOD_COMMAND = B_COMMAND_KEY;
const uint32 MOD_OPTION = B_OPTION_KEY;

/*
 * KeyboardShortcuts utility class
 * Provides helper methods for shortcut handling
 */
class KeyboardShortcuts {
public:
    // Check if a specific modifier combination is pressed
    static bool HasModifiers(uint32 modifiers, uint32 requiredMods);

    // Check if ONLY the specified modifiers are pressed (no extras)
    static bool HasExactModifiers(uint32 modifiers, uint32 requiredMods);

    // Get human-readable shortcut string for display
    static const char* GetShortcutString(char key, uint32 modifiers);

    // Check if key matches shortcut with modifiers
    static bool MatchesShortcut(char key, uint32 modifiers,
                                char expectedKey, uint32 expectedMods);
};

} // namespace HaikuDAW

#endif // KEYBOARD_SHORTCUTS_H
