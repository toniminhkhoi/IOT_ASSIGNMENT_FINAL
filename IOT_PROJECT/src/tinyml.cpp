#include "tinyml.h"
#include "dht_anomaly_model.h"
#include "global.h"

#include <math.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"

// Model array declared in the generated .h
extern const unsigned char dht_anomaly_model_tflite[];

// =====================================================
// TFLM internal globals
// =====================================================
namespace {
    tflite::ErrorReporter*    g_error_reporter = nullptr;
    const tflite::Model*      g_model         = nullptr;
    tflite::MicroInterpreter* g_interpreter    = nullptr;
    TfLiteTensor*             g_input          = nullptr;
    TfLiteTensor*             g_output         = nullptr;

    constexpr int kTensorArenaSize = 16 * 1024;
    alignas(16) static uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

// =====================================================
// INT8 quantization helpers
// =====================================================
static inline int8_t quantize(float x, float scale, int zp) {
    int v = (int)lroundf(x / scale + zp);
    if (v < -128) v = -128;
    if (v > 127)  v = 127;
    return (int8_t)v;
}

static inline float dequantize(int8_t q, float scale, int zp) {
    return (q - zp) * scale;
}

// Write 2-element input vector into the Tensor
static void write_input(TfLiteTensor* in, float temp, float hum) {
    if (in->type == kTfLiteInt8) {
        in->data.int8[0] = quantize(temp, in->params.scale, in->params.zero_point);
        in->data.int8[1] = quantize(hum,  in->params.scale, in->params.zero_point);
    } else {
        in->data.f[0] = temp;
        in->data.f[1] = hum;
    }
}

static float read_output_prob(const TfLiteTensor* out) {
    if (out->type == kTfLiteInt8) {
        return dequantize(out->data.int8[0], out->params.scale, out->params.zero_point);
    }
    return out->data.f[0];
}

// =====================================================
// Debug info
// =====================================================
static void print_model_info(TfLiteTensor* in, TfLiteTensor* out) {
    auto tstr = [](TfLiteType t) {
        switch (t) {
            case kTfLiteFloat32: return "F32";
            case kTfLiteInt8:    return "INT8";
            case kTfLiteUInt8:   return "UINT8";
            default:             return "OTHER";
        }
    };

    Serial.println("=== TFLM model info ===");

    Serial.printf("Input : type=%s dims=%d [", tstr(in->type), in->dims->size);
    for (int i = 0; i < in->dims->size; i++)
        Serial.printf("%d%s", in->dims->data[i], (i + 1 < in->dims->size) ? "," : "");
    Serial.printf("] scale=%.6f zp=%d\n", in->params.scale, in->params.zero_point);

    Serial.printf("Output: type=%s dims=%d [", tstr(out->type), out->dims->size);
    for (int i = 0; i < out->dims->size; i++)
        Serial.printf("%d%s", out->dims->data[i], (i + 1 < out->dims->size) ? "," : "");
    Serial.printf("] scale=%.6f zp=%d\n", out->params.scale, out->params.zero_point);

    Serial.println("========================");
}

// =====================================================
// Setup TinyML (runs once)
// =====================================================
void setupTinyML() {
    Serial.println("TensorFlow Lite Init...");

    static tflite::MicroErrorReporter micro_error_reporter;
    g_error_reporter = &micro_error_reporter;

    g_model = tflite::GetModel(dht_anomaly_model_tflite);
    if (g_model->version() != TFLITE_SCHEMA_VERSION) {
        g_error_reporter->Report("Model schema %d != supported %d",
                                 g_model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        g_model, resolver, tensor_arena, kTensorArenaSize, g_error_reporter);
    g_interpreter = &static_interpreter;

    if (g_interpreter->AllocateTensors() != kTfLiteOk) {
        g_error_reporter->Report("AllocateTensors() failed");
        return;
    }

    g_input  = g_interpreter->input(0);
    g_output = g_interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
    print_model_info(g_input, g_output);

    // Small sanity tests
    auto sanity = [&](float T, float H) {
        write_input(g_input, T, H);
        if (g_interpreter->Invoke() == kTfLiteOk) {
            float p = read_output_prob(g_output);
            Serial.printf("[Sanity] T=%.1f H=%.0f -> p=%.3f (%s)\n",
                          T, H, p, (p >= 0.5f) ? "ANOMALY" : "NORMAL");
        }
    };

    sanity(27.5f, 55.0f);
    sanity(32.0f, 55.0f);
    sanity(27.5f, 80.0f);
}

// =====================================================
// FreeRTOS Task
// =====================================================
void tiny_ml_task(void *pvParameters) {
    setupTinyML();

    if (!g_interpreter || !g_input || !g_output) {
        Serial.println("[TinyML] Setup failed; exiting task.");
        vTaskDelete(nullptr);
        return;
    }

    for (;;) {
        float T, H;

        // Safe sensor reading (mutex protected)
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            T = glob_temperature;
            H = glob_humidity;
            xSemaphoreGive(xDataMutex);
        } else {
            T = H = -1.0f;
        }

        bool valid = !isnan(T) && !isnan(H) &&
                     T >= -40.0f && T <= 125.0f &&
                     H >= 0.0f   && H <= 100.0f;

        if (valid) {
            write_input(g_input, T, H);

            if (g_interpreter->Invoke() == kTfLiteOk) {
                float p = read_output_prob(g_output);
                bool anomaly = (p >= 0.5f);

                Serial.printf("[TinyML] p=%.3f  %s  (T=%.1fC  H=%.0f%%)\n",
                              p, anomaly ? "ANOMALY" : "NORMAL", T, H);

                // CSV log line
                Serial.printf("%.1f,%.1f,%s\n", T, H,
                              anomaly ? "anomaly" : "normal");

            } else {
                Serial.println("[TinyML] Invoke failed");
            }
        } else {
            Serial.println("[TinyML] Invalid sensor data, skipping");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
