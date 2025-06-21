#!/usr/bin/python3
# Must be called from root directory
import shutil
import os
last_entry_for_key = {}

header = None

# Scan description.csv line-by-line
with open('./data/description.csv', 'r') as file:
    lines = file.readlines()
    for line in lines:
        if header is None:
            header = line.strip()
            continue
        # Split the line by comma
        fields = line.strip().split(',')
        key = ','.join(fields[:-1]) # The key is all the fields except for the last one (which is the data path)        
        value = fields[-1]
        if os.path.exists(value): # We might have entries without the corresponding folder
            if key in last_entry_for_key:
                # We found more recent data
                to_remove = last_entry_for_key[key]
                last_entry_for_key[key] = value # Update the last entry
                # Remove the old data
                shutil.rmtree(to_remove)
                print("Removed " + to_remove)
            else:                
                last_entry_for_key[key] = value # Update the last entry

# Now save the new description.csv
with open('./data/description.csv', 'w') as file:
    file.write(header + '\n')
    for k,v in last_entry_for_key.items():
        file.write(k + ',' + v + '\n')
    
