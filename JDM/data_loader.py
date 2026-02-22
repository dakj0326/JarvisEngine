import os
import numpy as np
import librosa
from librosa.feature import mfcc

SAMPLE_RATE = 16000
DURATION = 1.0
SAMPLES = int(SAMPLE_RATE * DURATION)
DATASET_PATH = "dataset"

TARGET_WORD = "yes"

def load_audio(file_path):
    audio, _ = librosa.load(file_path, sr=SAMPLE_RATE)

    if len(audio) > SAMPLES:
        audio = audio[:SAMPLES]
    else:
        padding = SAMPLES - len(audio)
        audio = np.pad(audio, (0, padding))

    return audio

def extract_mfcc(audio):
    return mfcc(y=audio, sr=SAMPLE_RATE, n_mfcc=40).T

def load_dataset(max_files_per_class=500):
    x = []
    y = []

    for folder in os.listdir(DATASET_PATH):
        folder_path = os.path.join(DATASET_PATH, folder)

        if not os.path.isdir(folder_path):
            print("Invalid path. Can probably be ignored")
            continue

        files = os.listdir(folder_path)[:max_files_per_class]

        for file in files:
            file_path = os.path.join(folder_path, file)

            audio = load_audio(file_path)
            features = extract_mfcc(audio)

            x.append(features)

            if folder == TARGET_WORD:
                y.append(0)  # yes
            else:
                y.append(1)  # unknown

    x = np.array(x)
    y = np.array(y)

    # Add channel dimension for CNN
    x = x[..., np.newaxis]

    return x, y