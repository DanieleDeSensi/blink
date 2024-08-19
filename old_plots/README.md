### Generate Plots

Generating plots requires three steps:

1. **Remove Previous PNG Files**  
   Delete existing PNG files to prevent any issues with the Python script:
   ```bash
   rm plots/*/*/*.png
   ```

2. **Generate CSV Files**  
   Run the `plot.sh` bash script to generate CSV files that collect all files related to the same experiment:
   ```bash
   ./plots/plot.sh
   ```

3. **Generate Plots**  
   Execute the `gen_plot.py` Python script to create the plots:
   ```bash
   python3 plots/gen_plot.py
   ```

---

### Explanation

1. **Remove Previous PNG Files**: Ensures that old plot images do not interfere with the new ones.
2. **Generate CSV Files**: Prepares the necessary data by aggregating relevant files for each experiment.
3. **Generate Plots**: Produces visualizations based on the aggregated data in the CSV files.

Follow these steps to generate your plots successfully.
