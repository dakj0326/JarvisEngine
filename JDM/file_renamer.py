import os
import time

folder = "recordings"

dest_jarvis = "dataset-google/jarvis"
name_jarvis = "jarvis_"

dest_similar = "dataset-google/jarvis_similar"
name_similar = "jarvis_similar_"


def get_last_index(dest, prefix):
    files = sorted(f for f in os.listdir(dest) if f.endswith(".wav"))
    if not files:
        return 0
    last_file = files[-1]
    number = last_file.replace(prefix, "").replace(".wav", "")
    return int(number)


jarvis_index = get_last_index(dest_jarvis, name_jarvis)
similar_index = get_last_index(dest_similar, name_similar)

jarvis = False
similar = True

name = name_jarvis
dest = dest_jarvis
index = jarvis_index

if similar:
    name = name_similar
    dest = dest_similar
    index = similar_index


while True:
    files = sorted(f for f in os.listdir(folder) if f.endswith(".wav"))

    if not files:
        time.sleep(1)
        continue

    old_path = os.path.join(folder, files[0])

    index += 1
    new_name = f"{name}{index:04}.wav"
    new_path = os.path.join(dest, new_name)

    os.rename(old_path, new_path)

    print(f"Moved {new_name}")