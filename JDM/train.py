
from JDM.data_loader import extract_mfcc, load_audio
from data_loader import load_dataset
import numpy as np
from model import build_model

print("Loading dataset...")
X, y = load_dataset(max_files_per_class=200)

print("X shape:", X.shape)
print("y shape:", y.shape)

model = build_model(input_shape=X.shape[1:], num_classes=2)

model.compile(
    optimizer='adam',
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

print("Training...")
model.fit(X, y, epochs=20, batch_size=32)

model.save("models/first_model.keras")


############################ TESTING ############################


print("\nTesting on a few samples... NOTE: 1 = not a keyword")




sample = extract_mfcc(
    load_audio("dataset/jarvis_similar/jarvis_similar_0001.wav")
)
sample = sample[..., np.newaxis]
sample = sample[np.newaxis, ...]
print("True Value:\t\t1")
print("Predicted Value:\t", model.predict(sample, verbose=0)[0])

sample = extract_mfcc(
    load_audio("dataset/jarvis/jarvis_0001.wav")
)
sample = sample[..., np.newaxis]
sample = sample[np.newaxis, ...]
print("True Value:\t\t0")
print("Predicted Value:\t", model.predict(sample, verbose=0)[0])

sample = extract_mfcc(
    load_audio("dataset/jarvis/jarvis_0050.wav")
)
sample = sample[..., np.newaxis]
sample = sample[np.newaxis, ...]
print("True Value:\t\t0")
print("Predicted Value:\t", model.predict(sample, verbose=0)[0])

sample = extract_mfcc(
    load_audio("dataset/jarvis/jarvis_0100.wav")
)
sample = sample[..., np.newaxis]
sample = sample[np.newaxis, ...]
print("True Value:\t\t0")
print("Predicted Value:\t", model.predict(sample, verbose=0)[0])