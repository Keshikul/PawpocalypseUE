"""Export Godot's input map (project.godot [input]) to Data/InputMap.csv: one row per event, Godot's and Unreal's names.

Called by tools/export_godot_data.py (so the L0 data-export gate checks it is current); runnable alone:

    py tools/export_input_map.py --godot C:/Games/pawpocalypse --out Data

Each Godot action becomes an Enhanced Input action IA_<action> (tools/create_input_assets.py) and each event one key mapping in
IMC_Pawpocalypse (docs/05 §3). Godot binds keys by PHYSICAL position (physical_keycode): every key row must have one.

Columns:
  id                <action>_<n>, n = the event's place in Godot's list
  action            the Godot action
  deadzone          the action's dead zone (Godot: an axis counts past it, and its strength is rescaled from it to 1)
  device            keyboard | mouse | pad
  godot_event       key | mouse_button | pad_button | pad_axis
  godot_code        physical_keycode / button_index / axis
  godot_axis_sign   -1 or 1 for a pad_axis (Godot: +Y is DOWN), else empty
  ue_key            the FKey name
  ue_axis_sign      the sign on Unreal's axis (Unreal: +Y is UP, so the Y axes flip), else empty
  merged            1 when Unreal has no key of its own and the row is another row's key (keypad Enter is Enter)
  _godot_file       project.godot
"""
from __future__ import annotations

import argparse
import csv
import os
import re
import sys

# Godot 4 Key (physical_keycode) -> Unreal FKey name. Only the keys the project binds; an unknown one fails the export.
KEYS = {
    32: "SpaceBar", 45: "Hyphen", 61: "Equals",
    49: "One", 50: "Two", 51: "Three", 52: "Four", 53: "Five", 54: "Six", 55: "Seven", 56: "Eight", 57: "Nine", 48: "Zero",
    **{c: chr(c) for c in range(65, 91)},  # A..Z
    4194305: "Escape", 4194306: "Tab", 4194308: "BackSpace", 4194309: "Enter", 4194310: "Enter",  # keypad Enter: no key of its own
    4194319: "Left", 4194320: "Up", 4194321: "Right", 4194322: "Down", 4194323: "PageUp", 4194324: "PageDown",
}
MERGED_KEYS = {4194310}

# Godot JoyButton -> Unreal (positions: A is the bottom face button on every pad).
PAD_BUTTONS = {
    0: "Gamepad_FaceButton_Bottom", 1: "Gamepad_FaceButton_Right", 2: "Gamepad_FaceButton_Left", 3: "Gamepad_FaceButton_Top",
    4: "Gamepad_Special_Left", 6: "Gamepad_Special_Right", 7: "Gamepad_LeftThumbstick", 8: "Gamepad_RightThumbstick",
    9: "Gamepad_LeftShoulder", 10: "Gamepad_RightShoulder", 11: "Gamepad_DPad_Up", 12: "Gamepad_DPad_Down",
    13: "Gamepad_DPad_Left", 14: "Gamepad_DPad_Right",
}
# Godot JoyAxis -> (Unreal axis key, sign flip): Godot's +Y is down, Unreal's +Y is up.
PAD_AXES = {
    0: ("Gamepad_LeftX", 1), 1: ("Gamepad_LeftY", -1), 2: ("Gamepad_RightX", 1), 3: ("Gamepad_RightY", -1),
    4: ("Gamepad_LeftTriggerAxis", 1), 5: ("Gamepad_RightTriggerAxis", 1),
}
MOUSE_BUTTONS = {1: "LeftMouseButton", 2: "RightMouseButton", 3: "MiddleMouseButton", 4: "MouseScrollUp", 5: "MouseScrollDown"}

COLUMNS = ["id", "action", "deadzone", "device", "godot_event", "godot_code", "godot_axis_sign", "ue_key", "ue_axis_sign", "merged",
           "_godot_file"]

RE_ACTION = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)=\{(.*?)^\}', re.S | re.M)
RE_EVENT = re.compile(r'Object\((InputEvent\w+),(.*?)\)\s*(?=,\s*Object|\])', re.S)


def _prop(props: str, key: str) -> str | None:
    m = re.search(r'"%s":([^,]+)' % re.escape(key), props)
    return m.group(1).strip() if m else None


def read_rows(godot: str) -> list[dict]:
    text = open(os.path.join(godot, "project.godot"), encoding="utf-8").read()
    start = text.index("\n[input]\n")
    end = text.find("\n[", start + 1)
    section = text[start:end if end >= 0 else len(text)]
    rows = []
    for action, body in RE_ACTION.findall(section):
        dz = re.search(r'"deadzone":\s*([0-9.]+)', body)
        deadzone = dz.group(1) if dz else "0.5"
        for n, (cls, props) in enumerate(RE_EVENT.findall(body)):
            row = {"id": "%s_%d" % (action, n), "action": action, "deadzone": deadzone, "godot_axis_sign": "", "ue_axis_sign": "",
                   "merged": "", "_godot_file": "project.godot"}
            if cls == "InputEventKey":
                for mod in ("shift_pressed", "ctrl_pressed", "alt_pressed", "meta_pressed"):
                    if _prop(props, mod) == "true":
                        raise SystemExit("export_input_map: %s binds a key with a modifier (%s): not supported" % (action, mod))
                code = int(_prop(props, "physical_keycode") or 0)
                if code == 0:
                    raise SystemExit("export_input_map: %s binds a key by keycode, not by position (physical_keycode 0)" % action)
                if code not in KEYS:
                    raise SystemExit("export_input_map: %s: unknown physical_keycode %d (add it to KEYS)" % (action, code))
                row.update(device="keyboard", godot_event="key", godot_code=str(code), ue_key=KEYS[code],
                           merged="1" if code in MERGED_KEYS else "")
            elif cls == "InputEventMouseButton":
                code = int(_prop(props, "button_index"))
                row.update(device="mouse", godot_event="mouse_button", godot_code=str(code), ue_key=MOUSE_BUTTONS[code])
            elif cls == "InputEventJoypadButton":
                code = int(_prop(props, "button_index"))
                if code not in PAD_BUTTONS:
                    raise SystemExit("export_input_map: %s: unknown joypad button %d" % (action, code))
                row.update(device="pad", godot_event="pad_button", godot_code=str(code), ue_key=PAD_BUTTONS[code])
            elif cls == "InputEventJoypadMotion":
                code = int(_prop(props, "axis"))
                sign = 1 if float(_prop(props, "axis_value")) > 0 else -1
                key, flip = PAD_AXES[code]
                row.update(device="pad", godot_event="pad_axis", godot_code=str(code), godot_axis_sign=str(sign), ue_key=key,
                           ue_axis_sign=str(sign * flip))
            else:
                raise SystemExit("export_input_map: %s: unsupported event %s" % (action, cls))
            rows.append(row)
    return rows


def export(godot: str, out: str) -> int:
    rows = read_rows(godot)
    path = os.path.join(out, "InputMap.csv")
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=COLUMNS)
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print("%-28s %4d rows -> %s (%d actions)" % ("InputMap", len(rows), path, len({r["action"] for r in rows})))
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    return export(a.godot, a.out)


if __name__ == "__main__":
    sys.exit(main())
