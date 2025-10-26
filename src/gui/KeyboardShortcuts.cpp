/*
 * KeyboardShortcuts.cpp - Implementation
 */

#include "KeyboardShortcuts.h"
#include <String.h>

namespace HaikuDAW {

bool KeyboardShortcuts::HasModifiers(uint32 modifiers, uint32 requiredMods)
{
    return (modifiers & requiredMods) == requiredMods;
}

bool KeyboardShortcuts::HasExactModifiers(uint32 modifiers, uint32 requiredMods)
{
    // Mask out non-modifier bits
    const uint32 modifierMask = B_SHIFT_KEY | B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY;
    uint32 actualMods = modifiers & modifierMask;

    return actualMods == requiredMods;
}

const char* KeyboardShortcuts::GetShortcutString(char key, uint32 modifiers)
{
    static BString shortcutStr;
    shortcutStr = "";

    // Add modifier symbols (in standard order)
    if (modifiers & B_CONTROL_KEY) {
        shortcutStr << "Ctrl+";
    }
    if (modifiers & B_OPTION_KEY) {
        shortcutStr << "Alt+";
    }
    if (modifiers & B_SHIFT_KEY) {
        shortcutStr << "Shift+";
    }
    if (modifiers & B_COMMAND_KEY) {
        shortcutStr << "Cmd+";
    }

    // Add key name
    switch (key) {
        case ' ':
            shortcutStr << "Space";
            break;
        case B_DELETE:
            shortcutStr << "Del";
            break;
        case B_ESCAPE:
            shortcutStr << "Esc";
            break;
        case B_HOME:
            shortcutStr << "Home";
            break;
        case B_LEFT_ARROW:
            shortcutStr << "Left";
            break;
        case B_RIGHT_ARROW:
            shortcutStr << "Right";
            break;
        case B_UP_ARROW:
            shortcutStr << "Up";
            break;
        case B_DOWN_ARROW:
            shortcutStr << "Down";
            break;
        default:
            if (key >= 'A' && key <= 'Z') {
                shortcutStr << (char)key;
            } else if (key >= 'a' && key <= 'z') {
                shortcutStr << (char)(key - 32);  // Convert to uppercase
            } else {
                shortcutStr << key;
            }
            break;
    }

    return shortcutStr.String();
}

bool KeyboardShortcuts::MatchesShortcut(char key, uint32 modifiers,
                                       char expectedKey, uint32 expectedMods)
{
    // Normalize key to uppercase for comparison
    char normalizedKey = key;
    if (key >= 'a' && key <= 'z') {
        normalizedKey = key - 32;
    }

    char normalizedExpected = expectedKey;
    if (expectedKey >= 'a' && expectedKey <= 'z') {
        normalizedExpected = expectedKey - 32;
    }

    // Check if key matches and modifiers match
    return (normalizedKey == normalizedExpected) &&
           HasExactModifiers(modifiers, expectedMods);
}

} // namespace HaikuDAW
