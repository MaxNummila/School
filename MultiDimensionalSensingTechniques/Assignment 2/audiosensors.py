import sys   # System manipulation
import time  # Used to pause execution in threads as needed
import keyboard  # Register keyboard events (keypresses)
import threading  # Threads for parallel execution
import pyaudio  # Audio streams
import numpy as np  # Matrix/list manipulation
import audioop  # Getting volume from sound data

from PyQt5.QtWidgets import QApplication, QLabel, QWidget
from PyQt5.QtGui import *

# Constants for streams, modify with care!
CHUNK = 1024 * 4
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100

print("Available audio devices:")
# Check the input devices
p = pyaudio.PyAudio()
info = p.get_host_api_info_by_index(0)
numdevices = info.get('deviceCount')
for i in range(0, numdevices):
    if (p.get_device_info_by_host_api_device_index(0, i)
            .get('maxInputChannels')) > 0:
        print("Input Device id ", i, " - ",
              p.get_device_info_by_host_api_device_index(0, i).get('name'))
p.terminate()


# This is the method running on a thread, one initialized for each audio device
def log_sound(index, label):
    global buffer  # buffer[index] is the list for this stream
    # wave_buffer = []      # Store audiosignal here

    # Open stream
    stream = p.open(
        format=FORMAT,
        channels=CHANNELS,
        rate=RATE,
        input=True,
        frames_per_buffer=CHUNK,
        input_device_index=index
    )

    while True:
        # Read a chunk of data from the stream
        data = stream.read(CHUNK)

        # Calculate the volume from the "chunk" of data
        volume = audioop.rms(data, 2)

        # Append the necessary data to the buffer
        buffer[index].append(volume)
        label.setText(f"{index}: {volume}")   # per-sensor volume

        # Check for quit command
        if keyboard.is_pressed('q') or quit_flag:
            print("Closing stream", index)
            stream.stop_stream()
            stream.close()
            break


def exitMethod():
    # Close threads when window is closed
    global quit_flag
    quit_flag = True


# This is the main thread, the code should be implemented here
def mainThread(mean_label, var_label):
    global buffer  # includes data from all audio sources

    buffer_width = 100  # how many latest entries we keep in each buffer

    while True:

        # Check the exit condition and join the threads if it is met
        if keyboard.is_pressed('q') or quit_flag:
            for x in threads:
                x.join()
            p.terminate()
            break

        # Limit buffers to the buffer_width
        for i in range(len(buffer)):
            buffer[i] = buffer[i][-buffer_width:]

        separateMean = []
        separateVar = []

        for i in range(len(buffer)):
            if buffer[i]:
                samples = np.array(buffer[i], dtype=float)
                meanVolume = float(np.mean(samples))
                varianceVolume = float(np.var(samples))
            else:
                meanVolume = 0.0
                varianceVolume = 0.0

            separateMean.append(meanVolume)
            separateVar.append(varianceVolume)

        allMics = []
        for i in range(len(buffer)):
            allMics.extend(buffer[i])

        if len(allMics) > 0:
            arr = np.array(allMics, dtype=float)
            combinedMean = np.mean(arr)
            combinedVarience = np.var(arr)

            mean_label.setText(f"Mean: {combinedMean:.2f}")
            var_label.setText(f"Variance: {combinedVarience:.2f}")

            faultySources = []
            lowMeanThreshold = 1.0

            for idx, meanVolume in enumerate(separateMean):
                if meanVolume < lowMeanThreshold:
                    faultySources.append(idx)

            if faultySources:
                print("Faulty sources (near zero for long time):", faultySources)
        else:
            mean_label.setText("Mean: 0")
            var_label.setText("Variance: 0")

        time.sleep(0.05)

    print("Execution finished")


# Store threads and labels
threads = []
labels = []
buffer = []
quit_flag = False

# GUI
app = QApplication(sys.argv)
app.aboutToQuit.connect(exitMethod)

# Initializing window
window = QWidget()
window.setWindowTitle('Soundwave log')
window.setGeometry(50, 50, 500, 500)
window.move(500, 500)

# Initialize pyaudio
p = pyaudio.PyAudio()
info = p.get_host_api_info_by_index(0)
numdevices = info.get('deviceCount')

# Run the threads
for i in range(0, numdevices):
    # Check if the device takes input
    if (p.get_device_info_by_host_api_device_index(0, i)
            .get('maxInputChannels')) > 0:

        # Initialize labels
        labels.append(QLabel("____________", parent=window))
        labels[-1].move(60, (15 * (i + 1)) + (10 * i))
        labels[-1].setFont(QFont('Arial', 10))

        # Append a new buffer to the global list
        buffer.append([])

        # Start threads
        threads.append(threading.Thread(target=log_sound, args=(i, labels[i])))
        threads[i].start()

# Init. labels for combined data
mean = QLabel("Mean: IMPLEMENT ME!", parent=window)
mean.move(60, (15 * numdevices + (10 * numdevices)))
mean.setFont(QFont('Arial', 12))

variance = QLabel("Variance: IMPLEMENT ME!", parent=window)
variance.move(60, (15 * numdevices + (13 * (numdevices + 2))))
variance.setFont(QFont('Arial', 12))

# Start the main thread
main_thread = threading.Thread(target=mainThread, args=[mean, variance])
main_thread.start()

# Show window
window.show()
app.exec_()
