#!/usr/bin/python3
# Must be called from root directory
import shutil
import os
last_entry_for_key = {}

# Scan description.csv line-by-line
with open('./data/description.csv', 'r') as file:
    lines = file.readlines()
    for line in lines:
        # Split the line by comma
        fields = line.strip().split(',')
        key = ' '.join(fields[:-1]) # The key is all the fields except for the last one (which is the data path)        
        value = fields[-1]
        if key in last_entry_for_key:
            # We found more recent data
            to_remove = last_entry_for_key[key]
            last_entry_for_key[key] = value # Update the last entry
            # Remove the old data
            shutil.rmtree(to_remove)
            print("Removed " + to_remove)
        elif os.exists(value):
            # No data yet, we store it only if the folder actually exists
            last_entry_for_key[key] = value # Update the last entry

# Now save the new description.csv
with open('./data/description.csv', 'w') as file:
    for k,v in last_entry_for_key.items():
        print(k + ',' + v + '\n')
        #file.write(k + ',' + v + '\n')
    
