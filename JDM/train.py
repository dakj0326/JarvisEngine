
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
model.fit(X, y, epochs=5, batch_size=32)

model.save("models/first_model.keras")


print("\nTesting on a few samples...")

for i in range(5):
    sample = X[i:i+1]  # keep batch dimension
    prediction = model.predict(sample, verbose=0)

    print("True label:", y[i])
    print("Predicted probabilities:", prediction[0])
    print("Predicted class:", np.argmax(prediction[0]))
    print("-----")