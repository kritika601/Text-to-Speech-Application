/* USER CODE BEGIN Header */
/**
  * STM32 TTS System - Edit the text below to change what it says!
  */
/* USER CODE END Header */

#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "es8388.h"
#include "tts_data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
// ============================================
// EDIT THIS LINE TO CHANGE WHAT IT SAYS:
// ============================================
const char* TEXT_TO_SPEAK = "Hello world, system is working.";
// ============================================

#define MAX_AUDIO_BUFFER 17640  // 400ms chunks (35KB RAM)
#define CHUNK_SIZE 4410         // Process 100ms at a time (8820 samples after upsampling)

static int16_t playbackBuffer[MAX_AUDIO_BUFFER];
static volatile uint8_t audio_busy = 0;

extern I2S_HandleTypeDef hi2s3;
extern UART_HandleTypeDef huart2;
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
void UART_Print(const char* msg);
int16_t* decode_audio(const char* word_or_letter, int* sample_count, uint8_t is_letter);
void play_audio(int16_t* mono_samples, int sample_count);
void speak_text(const char* text);
/* USER CODE END PFP */

void SystemClock_Config(void);

/* USER CODE BEGIN 0 */
void UART_Print(const char* msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);
}

void UART_PrintLine(const char* msg) {
    UART_Print(msg);
    UART_Print("\r\n");
}

// Decode ADPCM audio to PCM samples
int16_t* decode_audio(const char* word_or_letter, int* sample_count, uint8_t is_letter) {
    const uint8_t* compressed_data = NULL;
    int compressed_size = 0;
    int original_samples = 0;

    // Find the audio data
    if (is_letter) {
        char letter = toupper(word_or_letter[0]);

        // Match letter to compressed data
        if (letter == 'A') { compressed_data = tts_letter_a; compressed_size = TTS_LETTER_A_SIZE; original_samples = TTS_LETTER_A_SAMPLES; }
        else if (letter == 'B') { compressed_data = tts_letter_b; compressed_size = TTS_LETTER_B_SIZE; original_samples = TTS_LETTER_B_SAMPLES; }
        else if (letter == 'C') { compressed_data = tts_letter_c; compressed_size = TTS_LETTER_C_SIZE; original_samples = TTS_LETTER_C_SAMPLES; }
        else if (letter == 'D') { compressed_data = tts_letter_d; compressed_size = TTS_LETTER_D_SIZE; original_samples = TTS_LETTER_D_SAMPLES; }
        else if (letter == 'E') { compressed_data = tts_letter_e; compressed_size = TTS_LETTER_E_SIZE; original_samples = TTS_LETTER_E_SAMPLES; }
        else if (letter == 'F') { compressed_data = tts_letter_f; compressed_size = TTS_LETTER_F_SIZE; original_samples = TTS_LETTER_F_SAMPLES; }
        else if (letter == 'G') { compressed_data = tts_letter_g; compressed_size = TTS_LETTER_G_SIZE; original_samples = TTS_LETTER_G_SAMPLES; }
        else if (letter == 'H') { compressed_data = tts_letter_h; compressed_size = TTS_LETTER_H_SIZE; original_samples = TTS_LETTER_H_SAMPLES; }
        else if (letter == 'I') { compressed_data = tts_letter_i; compressed_size = TTS_LETTER_I_SIZE; original_samples = TTS_LETTER_I_SAMPLES; }
        else if (letter == 'J') { compressed_data = tts_letter_j; compressed_size = TTS_LETTER_J_SIZE; original_samples = TTS_LETTER_J_SAMPLES; }
        else if (letter == 'K') { compressed_data = tts_letter_k; compressed_size = TTS_LETTER_K_SIZE; original_samples = TTS_LETTER_K_SAMPLES; }
        else if (letter == 'L') { compressed_data = tts_letter_l; compressed_size = TTS_LETTER_L_SIZE; original_samples = TTS_LETTER_L_SAMPLES; }
        else if (letter == 'M') { compressed_data = tts_letter_m; compressed_size = TTS_LETTER_M_SIZE; original_samples = TTS_LETTER_M_SAMPLES; }
        else if (letter == 'N') { compressed_data = tts_letter_n; compressed_size = TTS_LETTER_N_SIZE; original_samples = TTS_LETTER_N_SAMPLES; }
        else if (letter == 'O') { compressed_data = tts_letter_o; compressed_size = TTS_LETTER_O_SIZE; original_samples = TTS_LETTER_O_SAMPLES; }
        else if (letter == 'P') { compressed_data = tts_letter_p; compressed_size = TTS_LETTER_P_SIZE; original_samples = TTS_LETTER_P_SAMPLES; }
        else if (letter == 'Q') { compressed_data = tts_letter_q; compressed_size = TTS_LETTER_Q_SIZE; original_samples = TTS_LETTER_Q_SAMPLES; }
        else if (letter == 'R') { compressed_data = tts_letter_r; compressed_size = TTS_LETTER_R_SIZE; original_samples = TTS_LETTER_R_SAMPLES; }
        else if (letter == 'S') { compressed_data = tts_letter_s; compressed_size = TTS_LETTER_S_SIZE; original_samples = TTS_LETTER_S_SAMPLES; }
        else if (letter == 'T') { compressed_data = tts_letter_t; compressed_size = TTS_LETTER_T_SIZE; original_samples = TTS_LETTER_T_SAMPLES; }
        else if (letter == 'U') { compressed_data = tts_letter_u; compressed_size = TTS_LETTER_U_SIZE; original_samples = TTS_LETTER_U_SAMPLES; }
        else if (letter == 'V') { compressed_data = tts_letter_v; compressed_size = TTS_LETTER_V_SIZE; original_samples = TTS_LETTER_V_SAMPLES; }
        else if (letter == 'W') { compressed_data = tts_letter_w; compressed_size = TTS_LETTER_W_SIZE; original_samples = TTS_LETTER_W_SAMPLES; }
        else if (letter == 'X') { compressed_data = tts_letter_x; compressed_size = TTS_LETTER_X_SIZE; original_samples = TTS_LETTER_X_SAMPLES; }
        else if (letter == 'Y') { compressed_data = tts_letter_y; compressed_size = TTS_LETTER_Y_SIZE; original_samples = TTS_LETTER_Y_SAMPLES; }
        else if (letter == 'Z') { compressed_data = tts_letter_z; compressed_size = TTS_LETTER_Z_SIZE; original_samples = TTS_LETTER_Z_SAMPLES; }
    } else {
        // Match word to compressed data
        if (strcmp(word_or_letter, "hello") == 0) { compressed_data = tts_word_hello; compressed_size = TTS_WORD_HELLO_SIZE; original_samples = TTS_WORD_HELLO_SAMPLES; }
        else if (strcmp(word_or_letter, "ready") == 0) { compressed_data = tts_word_ready; compressed_size = TTS_WORD_READY_SIZE; original_samples = TTS_WORD_READY_SAMPLES; }
        else if (strcmp(word_or_letter, "error") == 0) { compressed_data = tts_word_error; compressed_size = TTS_WORD_ERROR_SIZE; original_samples = TTS_WORD_ERROR_SAMPLES; }
        else if (strcmp(word_or_letter, "warning") == 0) { compressed_data = tts_word_warning; compressed_size = TTS_WORD_WARNING_SIZE; original_samples = TTS_WORD_WARNING_SAMPLES; }
        else if (strcmp(word_or_letter, "system") == 0) { compressed_data = tts_word_system; compressed_size = TTS_WORD_SYSTEM_SIZE; original_samples = TTS_WORD_SYSTEM_SAMPLES; }
        else if (strcmp(word_or_letter, "sensor") == 0) { compressed_data = tts_word_sensor; compressed_size = TTS_WORD_SENSOR_SIZE; original_samples = TTS_WORD_SENSOR_SAMPLES; }
        else if (strcmp(word_or_letter, "temperature") == 0) { compressed_data = tts_word_temperature; compressed_size = TTS_WORD_TEMPERATURE_SIZE; original_samples = TTS_WORD_TEMPERATURE_SAMPLES; }
        else if (strcmp(word_or_letter, "working") == 0) { compressed_data = tts_word_working; compressed_size = TTS_WORD_WORKING_SIZE; original_samples = TTS_WORD_WORKING_SAMPLES; }
        else if (strcmp(word_or_letter, "pressure") == 0) { compressed_data = tts_word_pressure; compressed_size = TTS_WORD_PRESSURE_SIZE; original_samples = TTS_WORD_PRESSURE_SAMPLES; }
        else if (strcmp(word_or_letter, "battery") == 0) { compressed_data = tts_word_battery; compressed_size = TTS_WORD_BATTERY_SIZE; original_samples = TTS_WORD_BATTERY_SAMPLES; }
        else if (strcmp(word_or_letter, "degrees") == 0) { compressed_data = tts_word_degrees; compressed_size = TTS_WORD_DEGREES_SIZE; original_samples = TTS_WORD_DEGREES_SAMPLES; }
        else if (strcmp(word_or_letter, "celsius") == 0) { compressed_data = tts_word_celsius; compressed_size = TTS_WORD_CELSIUS_SIZE; original_samples = TTS_WORD_CELSIUS_SAMPLES; }
        else if (strcmp(word_or_letter, "percent") == 0) { compressed_data = tts_word_percent; compressed_size = TTS_WORD_PERCENT_SIZE; original_samples = TTS_WORD_PERCENT_SAMPLES; }
        else if (strcmp(word_or_letter, "high") == 0) { compressed_data = tts_word_high; compressed_size = TTS_WORD_HIGH_SIZE; original_samples = TTS_WORD_HIGH_SAMPLES; }
        else if (strcmp(word_or_letter, "low") == 0) { compressed_data = tts_word_low; compressed_size = TTS_WORD_LOW_SIZE; original_samples = TTS_WORD_LOW_SAMPLES; }
        else if (strcmp(word_or_letter, "is") == 0) { compressed_data = tts_word_is; compressed_size = TTS_WORD_IS_SIZE; original_samples = TTS_WORD_IS_SAMPLES; }
        else if (strcmp(word_or_letter, "and") == 0) { compressed_data = tts_word_and; compressed_size = TTS_WORD_AND_SIZE; original_samples = TTS_WORD_AND_SAMPLES; }
        else if (strcmp(word_or_letter, "the") == 0) { compressed_data = tts_word_the; compressed_size = TTS_WORD_THE_SIZE; original_samples = TTS_WORD_THE_SAMPLES; }
        else if (strcmp(word_or_letter, "on") == 0) { compressed_data = tts_word_on; compressed_size = TTS_WORD_ON_SIZE; original_samples = TTS_WORD_ON_SAMPLES; }
        else if (strcmp(word_or_letter, "off") == 0) { compressed_data = tts_word_off; compressed_size = TTS_WORD_OFF_SIZE; original_samples = TTS_WORD_OFF_SAMPLES; }
        else if (strcmp(word_or_letter, "zero") == 0) { compressed_data = tts_word_zero; compressed_size = TTS_WORD_ZERO_SIZE; original_samples = TTS_WORD_ZERO_SAMPLES; }
        else if (strcmp(word_or_letter, "one") == 0) { compressed_data = tts_word_one; compressed_size = TTS_WORD_ONE_SIZE; original_samples = TTS_WORD_ONE_SAMPLES; }
        else if (strcmp(word_or_letter, "two") == 0) { compressed_data = tts_word_two; compressed_size = TTS_WORD_TWO_SIZE; original_samples = TTS_WORD_TWO_SAMPLES; }
        else if (strcmp(word_or_letter, "three") == 0) { compressed_data = tts_word_three; compressed_size = TTS_WORD_THREE_SIZE; original_samples = TTS_WORD_THREE_SAMPLES; }
        else if (strcmp(word_or_letter, "four") == 0) { compressed_data = tts_word_four; compressed_size = TTS_WORD_FOUR_SIZE; original_samples = TTS_WORD_FOUR_SAMPLES; }
        else if (strcmp(word_or_letter, "five") == 0) { compressed_data = tts_word_five; compressed_size = TTS_WORD_FIVE_SIZE; original_samples = TTS_WORD_FIVE_SAMPLES; }
        else if (strcmp(word_or_letter, "six") == 0) { compressed_data = tts_word_six; compressed_size = TTS_WORD_SIX_SIZE; original_samples = TTS_WORD_SIX_SAMPLES; }
        else if (strcmp(word_or_letter, "seven") == 0) { compressed_data = tts_word_seven; compressed_size = TTS_WORD_SEVEN_SIZE; original_samples = TTS_WORD_SEVEN_SAMPLES; }
        else if (strcmp(word_or_letter, "eight") == 0) { compressed_data = tts_word_eight; compressed_size = TTS_WORD_EIGHT_SIZE; original_samples = TTS_WORD_EIGHT_SAMPLES; }
        else if (strcmp(word_or_letter, "nine") == 0) { compressed_data = tts_word_nine; compressed_size = TTS_WORD_NINE_SIZE; original_samples = TTS_WORD_NINE_SAMPLES; }
        else if (strcmp(word_or_letter, "ten") == 0) { compressed_data = tts_word_ten; compressed_size = TTS_WORD_TEN_SIZE; original_samples = TTS_WORD_TEN_SAMPLES; }
        else if (strcmp(word_or_letter, "eleven") == 0) { compressed_data = tts_word_eleven; compressed_size = TTS_WORD_ELEVEN_SIZE; original_samples = TTS_WORD_ELEVEN_SAMPLES; }
        else if (strcmp(word_or_letter, "twelve") == 0) { compressed_data = tts_word_twelve; compressed_size = TTS_WORD_TWELVE_SIZE; original_samples = TTS_WORD_TWELVE_SAMPLES; }
        else if (strcmp(word_or_letter, "thirteen") == 0) { compressed_data = tts_word_thirteen; compressed_size = TTS_WORD_THIRTEEN_SIZE; original_samples = TTS_WORD_THIRTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "fourteen") == 0) { compressed_data = tts_word_fourteen; compressed_size = TTS_WORD_FOURTEEN_SIZE; original_samples = TTS_WORD_FOURTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "fifteen") == 0) { compressed_data = tts_word_fifteen; compressed_size = TTS_WORD_FIFTEEN_SIZE; original_samples = TTS_WORD_FIFTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "sixteen") == 0) { compressed_data = tts_word_sixteen; compressed_size = TTS_WORD_SIXTEEN_SIZE; original_samples = TTS_WORD_SIXTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "seventeen") == 0) { compressed_data = tts_word_seventeen; compressed_size = TTS_WORD_SEVENTEEN_SIZE; original_samples = TTS_WORD_SEVENTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "eighteen") == 0) { compressed_data = tts_word_eighteen; compressed_size = TTS_WORD_EIGHTEEN_SIZE; original_samples = TTS_WORD_EIGHTEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "nineteen") == 0) { compressed_data = tts_word_nineteen; compressed_size = TTS_WORD_NINETEEN_SIZE; original_samples = TTS_WORD_NINETEEN_SAMPLES; }
        else if (strcmp(word_or_letter, "twenty") == 0) { compressed_data = tts_word_twenty; compressed_size = TTS_WORD_TWENTY_SIZE; original_samples = TTS_WORD_TWENTY_SAMPLES; }
    }

    if (!compressed_data) {
        *sample_count = 0;
        return NULL;
    }

    // Allocate output buffer
    int16_t* output = (int16_t*)malloc(original_samples * sizeof(int16_t));
    if (!output) {
        *sample_count = 0;
        return NULL;
    }

    // Decode ADPCM
    adpcm_decoder_t decoder;
    adpcm_decoder_init(&decoder);

    int out_pos = 0;
    for (int i = 0; i < compressed_size && out_pos < original_samples; i++) {
        if (out_pos < original_samples) {
            output[out_pos++] = adpcm_decode_sample(&decoder, compressed_data[i] & 0xF);
        }
        if (out_pos < original_samples) {
            output[out_pos++] = adpcm_decode_sample(&decoder, (compressed_data[i] >> 4) & 0xF);
        }
    }

    *sample_count = out_pos;
    return output;
}

// Play audio in chunks to avoid huge buffer
void play_audio(int16_t* mono_samples, int sample_count) {
    if (!mono_samples || sample_count == 0) return;

    char msg[80];
    sprintf(msg, "  Playing %d samples (%.1f ms) in chunks\r\n",
            sample_count, (sample_count * 1000.0f) / 22050);
    UART_Print(msg);

    // Process audio in chunks
    int samples_processed = 0;

    while (samples_processed < sample_count) {
        // How many samples in this chunk?
        int chunk_samples = CHUNK_SIZE;
        if (samples_processed + chunk_samples > sample_count) {
            chunk_samples = sample_count - samples_processed;
        }

        // Convert this chunk: mono 22kHz → stereo 44kHz
        int buffer_pos = 0;
        for (int i = 0; i < chunk_samples; i++) {
            int16_t sample = mono_samples[samples_processed + i];

            // Each sample becomes 4 output samples (2x rate, 2x channels)
            playbackBuffer[buffer_pos++] = sample;  // Left 1
            playbackBuffer[buffer_pos++] = sample;  // Right 1
            playbackBuffer[buffer_pos++] = sample;  // Left 2 (upsample)
            playbackBuffer[buffer_pos++] = sample;  // Right 2 (upsample)
        }

        // Play this chunk via I2S DMA
        audio_busy = 1;
        HAL_StatusTypeDef status = HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)playbackBuffer, buffer_pos);

        if (status != HAL_OK) {
            UART_Print("  ERROR: DMA failed!\r\n");
            audio_busy = 0;
            return;
        }

        // Wait for this chunk to finish
        int timeout = 2000;
        while (audio_busy && timeout > 0) {
            HAL_Delay(1);
            timeout--;
        }

        if (timeout == 0) {
            UART_Print("  WARNING: Chunk timeout!\r\n");
            audio_busy = 0;
            return;
        }

        samples_processed += chunk_samples;
    }
}

// Speak entire text
void speak_text(const char* text) {
    char word[64];
    int word_pos = 0;

    UART_Print("\r\n========================================\r\n");
    UART_Print("Speaking: \"");
    UART_Print(text);
    UART_Print("\"\r\n");
    UART_Print("========================================\r\n");

    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];

        // Check if this is a word boundary (space, punctuation, or any non-alphanumeric)
        if (!isalnum(c)) {
            if (word_pos > 0) {
                word[word_pos] = '\0';

                // Try to find word in vocabulary
                int sample_count = 0;
                int16_t* samples = decode_audio(word, &sample_count, 0);

                if (samples) {
                    // Word found - speak
                    char msg[80];
                    sprintf(msg, "\r\nWord: '%s'\r\n", word);
                    UART_Print(msg);
                    play_audio(samples, sample_count);
                    free(samples);
                    HAL_Delay(200);  // Pause between words
                } else {
                    // Word not found - pronounce it phonetically
                    char msg[80];
                    sprintf(msg, "\r\nPhonetic: '%s' → ", word);
                    UART_Print(msg);

                    for (int j = 0; word[j] != '\0'; j++) {
                        char letter_c = word[j];
                        if (isalpha((unsigned char)letter_c)) {
                            // Use letter phonetics
                            char letter_str[2] = {letter_c, '\0'};
                            samples = decode_audio(letter_str, &sample_count, 1);
                            if (samples) {
                                // Print letter for debug
                                char letter_debug[3] = {letter_c, '-', '\0'};
                                UART_Print(letter_debug);

                                play_audio(samples, sample_count);
                                free(samples);
                                HAL_Delay(60);
                            }
                        }
                    }
                    UART_Print("\r\n");
                    HAL_Delay(150);
                }

                word_pos = 0;
            }

            if (c == '.' || c == '!' || c == '?') {
                HAL_Delay(400);  // Extra pause at end of sentence
            }
        } else {
            // Alphanumeric character - add to current word
            if (word_pos < 63) {
                word[word_pos++] = tolower(c);
            }
        }
    }

    // Speak last word (if text doesn't end with punctuation)
    if (word_pos > 0) {
        word[word_pos] = '\0';
        int sample_count = 0;
        int16_t* samples = decode_audio(word, &sample_count, 0);
        if (samples) {
            char msg[80];
            sprintf(msg, "\r\nWord: '%s'\r\n", word);
            UART_Print(msg);
            play_audio(samples, sample_count);
            free(samples);
        } else {
            char msg[80];
            sprintf(msg, "\r\nPhonetic: '%s' → ", word);
            UART_Print(msg);

            for (int j = 0; word[j] != '\0'; j++) {
                char letter_c = word[j];
                if (isalpha((unsigned char)letter_c)) {
                    char letter_str[2] = {letter_c, '\0'};
                    samples = decode_audio(letter_str, &sample_count, 1);
                    if (samples) {
                        char letter_debug[3] = {letter_c, '-', '\0'};
                        UART_Print(letter_debug);

                        play_audio(samples, sample_count);
                        free(samples);
                        HAL_Delay(60);
                    }
                }
            }
            UART_Print("\r\n");
        }
    }

    UART_Print("\r\n========================================\r\n");
    UART_Print("Done!\r\n");
    UART_Print("========================================\r\n\r\n");
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI3) {
        audio_busy = 0;  // Signal that DMA transfer is complete
    }
}
/* USER CODE END 0 */

int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  UART_Print("\r\n");
  UART_Print("=========================================\r\n");
  UART_Print("    STM32 TTS SYSTEM - FINAL VERSION\r\n");
  UART_Print("=========================================\r\n\r\n");

  // Initialize ES8388 codec
  UART_Print("Initializing ES8388...\r\n");
  ES8388_Init();
  HAL_Delay(100);

  // Start DAC
  es8388_start(ES_MODULE_DAC);
  HAL_Delay(500);

  UART_Print("TTS Ready!\r\n\r\n");

  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Speak the text
    speak_text(TEXT_TO_SPEAK);

    // Wait 5 seconds before repeating
    UART_Print("Waiting 5 seconds...\r\n\r\n");
    HAL_Delay(5000);
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
