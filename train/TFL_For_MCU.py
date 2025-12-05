import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report
import tensorflow as tf

tf.keras.backend.clear_session()

PREFIX = "dht_anomaly_model"

# 1. Load dataset (CSV has header: temp,humidity,label)
data = pd.read_csv("real_sensor_data.csv")

print("=== DEBUG: Dataset shape ===")
print(data.shape)

print("=== DEBUG: First 5 rows ===")
print(data.head())

print("=== DEBUG: Label distribution ===")
print(data["label"].value_counts())


X = data[["temp", "humidity"]].values.astype("float32")
y = data["label"].values.astype("float32")

# 2. Train / test split
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, shuffle=True
)

# 3. Define a simple TinyML-friendly model
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(2,)),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])

model.compile(
    loss="binary_crossentropy",
    optimizer="adam",
    metrics=["accuracy"]
)

# 4. Train
history = model.fit(
    X_train, y_train,
    epochs=50,
    batch_size=16,
    validation_data=(X_test, y_test),
    verbose=1
)

# 5. Evaluate on test set
y_pred_prob = model.predict(X_test)
y_pred = (y_pred_prob >= 0.5).astype("float32")

acc = accuracy_score(y_test, y_pred)
print("\n=== Test Accuracy on PC ===")
print(f"Accuracy: {acc * 100:.2f}%")
print("\nClassification report:")
print(classification_report(y_test, y_pred, digits=3))

# 6. Save Keras model
model.save(PREFIX + ".h5")

# 7. Convert to TFLite (optimized)
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

# 8. Dump TFLite into C header for MCU
tflite_path = PREFIX + ".tflite"
output_header_path = PREFIX + ".h"

with open(tflite_path, "rb") as tflite_file:
    tflite_content = tflite_file.read()

hex_lines = [
    ", ".join([f"0x{byte:02x}" for byte in tflite_content[i:i + 12]])
    for i in range(0, len(tflite_content), 12)
]
hex_array = ",\n  ".join(hex_lines)

with open(output_header_path, "w") as header_file:
    header_file.write("const unsigned char dht_anomaly_model_tflite[] = {\n  ")
    header_file.write(f"{hex_array}\n")
    header_file.write("};\n\n")
    header_file.write(f"const unsigned int model_len = {len(tflite_content)};\n")
