import csv
import matplotlib.pyplot as plt
import os
import sys

# Function to parse CSV file
def parse_csv(filename):
    with open(filename, 'r') as file:
        reader = csv.reader(file)
        data = [row for row in reader]
    return data

def find_files(directory):
    paths = []
    # Define the file type you want to iterate over
    file_type = '.csv'

    # Iterate over each file in the directory
    for filename in os.listdir(directory):
        if filename.endswith(file_type):
            file_path = os.path.join(directory, filename)
            paths.append(file_path)

    return paths

def get_all_folders(path):
  """
  This function retrieves all folders (subdirectories) within a path and its subdirectories.

  Args:
      path: The directory path to start searching.

  Returns:
      A list containing all folder paths within the specified path and its subdirectories.
  """
  all_folders = []
  for dirpath, dirnames, _ in os.walk(path):
    for dirname in dirnames:
      folder_path = os.path.join(dirpath, dirname)
      all_folders.append(folder_path)
  return all_folders

# Main function
def main():

    if (len(sys.argv) < 2):
        print("Pass the name of the test as an argument please.")
        return 

    test_name = sys.argv[1]

    print(os.getcwd())
    # Indexes
    AvgFrameTimeIndex = 2
    MaxFrameTimeIndex = 3

    MinRTIndex = 4
    AvgRTIndex = 5
    MaxRTIndex = 6

    MinGTIndex = 7
    AvgGTIndex = 8
    MaxGTIndex = 9

    MinGPUIndex = 10
    AvgGPUIndex = 11
    MaxGPUIndex = 12

    avg_ft = []
    max_ft = []

    min_rt = []
    avg_rt = []
    max_rt = []

    min_gt = []
    avg_gt = []    
    max_gt = []

    min_gpu = []    
    avg_gpu = []    
    max_gpu = []    

  #  print(get_all_folders(os.getcwd() + "/Profiling/PerformanceTest01/"))
    folders = get_all_folders(os.getcwd() + "/Saved/Profiling/" + test_name + "/")
    paths = []
    for folder in folders:
        for file in find_files(folder):
            paths.append(file)
        #paths += find_files(os.getcwd() + "/Profiling/PerformanceTest01/UEDPIE_0_TBRaymarcherShowcase-WindowsEditor-03.10-12.17.12/")
    
    # Sort the files by time they were created.
    paths.sort(key=lambda x: os.path.getctime(x))
    
    paths = paths[-10:] if len(paths) > 10 else paths

    xRange = [x for x in range(0, len(paths))]
    for path in paths:
        # Load data from CSV file
        data = parse_csv(path)
        # Find the index of the Adjusted Results section
        adjusted_index = data.index(['Adjusted Results'])
        # Extract data for plotting from the Adjusted Results section
        adjusted_data = data[adjusted_index + 2:adjusted_index + 3][0]  # Skip header and separator
        test_names = adjusted_data[0]
        adjusted_data = adjusted_data[1:]
        adjusted_data = [float(x) for x in adjusted_data]
        adjusted_data.insert(0, test_names)

        min_rt.append(adjusted_data[MinRTIndex])
        avg_rt.append(adjusted_data[AvgRTIndex])
        max_rt.append(adjusted_data[MaxRTIndex]) 

        min_gt.append(adjusted_data[MinGTIndex])
        avg_gt.append(adjusted_data[AvgGTIndex])
        max_gt.append(adjusted_data[MaxGTIndex])

        min_gpu.append(adjusted_data[MinGPUIndex])
        avg_gpu.append(adjusted_data[AvgGPUIndex])
        max_gpu.append(adjusted_data[MaxGPUIndex])
        
        avg_ft.append(adjusted_data[AvgFrameTimeIndex])
        max_ft.append(adjusted_data[MaxFrameTimeIndex])

    fig, ax = plt.subplots(nrows=3, ncols=2)
    
    # Plot the data
    ax[0][0].scatter(xRange, min_rt, label="MinRT")
    ax[0][0].scatter(xRange, avg_rt, label='AvgRT')
    ax[0][0].scatter(xRange, max_rt, label='MaxRT')
    ax[0][0].set_title('Render Thread performance. [ms]')
    ax[0][0].set_xlim(-0.5, len(paths) + 0.5)
    ax[0][0].legend()

    ax[1][1].scatter(xRange, max_ft, label='MaxFT')
    ax[1][1].set_title('Max frame time. [ms]')
    ax[1][1].set_xlim(-0.5, len(paths) + 0.5)
    ax[1][1].legend()

    ax[2][0].scatter(xRange, avg_ft, label='AvgFT')
    ax[2][0].set_title('Avg frame time. [ms]')
    ax[2][0].set_xlim(-0.5, len(paths) + 0.5)
    ax[2][0].legend()

    ax[0][1].scatter(xRange, min_gt, label="MinGT")
    ax[0][1].scatter(xRange, avg_gt, label='AvgGT')
    ax[0][1].scatter(xRange, max_gt, label='MaxGT')
    ax[0][1].set_title('Game Thread performance. [ms]')
    ax[0][1].set_xlim(-0.5, len(paths) + 0.5)
    ax[0][1].legend()
    
    ax[1][0].scatter(xRange, min_gpu, label="MinGPU")
    ax[1][0].scatter(xRange, avg_gpu, label='AvgGPU')
    ax[1][0].scatter(xRange, max_gpu, label='MaxGPU')
    ax[1][0].set_title('GPU performance. [ms]')
    ax[1][0].set_xlim(-0.5, len(paths) + 0.5)
    ax[1][0].legend()

    
    plt.ylabel('Values in [ms]')
    
    #plt.title('Adjusted Results: MaxRT, MaxGT, and MaxGPU')
    #plt.xticks(rotation=45, ha='right')  # Rotate x-axis labels for better visibility
    
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
