import json
import os

folder_path = "data\\markers"
generated_file = "src\\generated_macros.hpp"
macros = []

# Get all filenames in the folder (excluding directories)
filenames = [filename for filename in os.listdir(folder_path) if os.path.isfile(os.path.join(folder_path, filename))]

def format_macro(location, name):
  return '#define CB_{}_{} "{}"\n'.format(location.upper(), name.upper(), name)

def generate_macros(filename):
  with open(filename, "r") as file:
    data = json.load(file)
    identifier = data["identifier"]
    markers = data["markers"]
    macros.append('\n// LOCATION {} MACROS\n'.format(filename))
    macros.append(format_macro("location_" + identifier["location"], identifier["name"]))
    for marker in markers:
      macros.append(format_macro("position_" + marker["location"], marker["name"]))

for filename in filenames:
  generate_macros(os.path.join(folder_path, filename))

with open(generated_file, "w") as file:
  file.write('// DO NOT CHANGE THIS FILE, THIS FILE IS BEING AUTOMATICALLY GENERATED BY generate_macros.py\n')
  file.write("#pragma once\n\n")

  file.write('// ACTION MACROS\n')
  file.write('#define CB_ACTION_NONE "none"\n')
  file.write('#define CB_ACTION_REFRESH_SWORDS "refresh_swords"\n')
  file.write('#define CB_ACTION_REFRESH_POTIONS "refresh_potions"\n')
  file.write('#define CB_ACTION_UPDATE_EXP "update_exp"\n')
  file.write('#define CB_ACTION_REFRESH_BUFFS_INVENTORY "refresh_buffs_inventory"\n')
  file.write('#define CB_ACTION_REFRESH_BUFFS_RETURN "refresh_buffs_return"\n')

  file.write('\n')

  file.write('// ROUTINE MACROS\n')
  file.write('#define CB_ROUTINE_NONE "none"\n')
  file.write('#define CB_ROUTINE_FARM "farm"\n')
  file.write('#define CB_LOCATION_UNKNOWN "unknown"\n')

  file.write('\n')

  file.write('// REFRESH MODES MACROS\n')
  file.write('#define CB_REFRESH_MODE_LOGOUT "logout"\n')
  file.write('#define CB_REFRESH_MODE_CLOSE "close"\n')

  for macro in macros:
    file.write(macro)