#include "es8388.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

void UART_PrintLine(const char* msg);

static HAL_StatusTypeDef ES8388_Write(uint8_t reg, uint8_t val)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&hi2c1, ES8388_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 1000);
    
    if (status != HAL_OK) {
        char buf[60];
        sprintf(buf, "I2C WRITE FAILED: reg=0x%02X", reg);
        UART_PrintLine(buf);
        return status;
    }
    
    HAL_Delay(5);
    
    // Verify critical registers
    if (reg == 0x19 || reg == 0x1A || reg == 0x1B || reg == 0x2E || reg == 0x2F) {
        HAL_Delay(5);
        uint8_t readback = ES8388_Read(reg);
        if (readback != val) {
            char buf[80];
            sprintf(buf, "VERIFY FAIL: 0x%02X wrote 0x%02X, read 0x%02X", reg, val, readback);
            UART_PrintLine(buf);
        } else {
            char buf[80];
            sprintf(buf, "VERIFY OK: 0x%02X = 0x%02X", reg, val);
            UART_PrintLine(buf);
        }
    }
    
    return status;
}

uint8_t ES8388_Read(uint8_t reg)
{
    uint8_t val = 0;
    HAL_I2C_Mem_Read(&hi2c1, ES8388_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 1000);
    HAL_Delay(2);
    return val;
}

void ES8388_PrintRegisters(void)
{
    char buf[80];
    
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n=== ES8388 Register Dump ===\r\n", 32, 1000);
    
    uint8_t regs[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x08, 0x09, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x27, 0x2A, 0x2E, 0x2F};
    char* names[] = {"CTRL1", "CTRL2", "PWRMAN", "ADCPWR", "DACPWR", "MASTER", "ADCCTL5", "DACCTL1", "DACCTL2", "DACCTL3", "DACCTL4", "DACCTL5", "DACCTL17", "DACCTL20", "DACCTL24", "DACCTL25"};
    
    for (int i = 0; i < 16; i++) {
        uint8_t val = ES8388_Read(regs[i]);
        sprintf(buf, "%s (0x%02X) = 0x%02X\r\n", names[i], regs[i], val);
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);
    }
    
    HAL_UART_Transmit(&huart2, (uint8_t*)"=== End Register Dump ===\r\n\r\n", 30, 1000);
}

void ES8388_Init(void)
{
    UART_PrintLine("===========================================");
    UART_PrintLine("ES8388 Init - EXACT ESP-ADF Sequence");
    UART_PrintLine("===========================================");
    
    // Following ESP-ADF es8388.c EXACTLY line by line
    
    // Initial mute (DACCONTROL3)
    ES8388_Write(ES8388_DACCONTROL3, 0x04);
    
    /* Chip Control and Power Management */
    ES8388_Write(ES8388_CONTROL2, 0x50);
    ES8388_Write(ES8388_CHIPPOWER, 0x00);  // normal all and power up all
    HAL_Delay(50);
    
    // Disable the internal DLL to improve 8K sample rate
    ES8388_Write(0x35, 0xA0);
    ES8388_Write(0x37, 0xD0);
    ES8388_Write(0x39, 0xD0);
    
    ES8388_Write(ES8388_MASTERMODE, 0x00);  // CODEC IN I2S SLAVE MODE
    
    /* DAC */
    ES8388_Write(ES8388_DACPOWER, 0xC0);  // disable DAC and disable Lout/Rout/1/2
    ES8388_Write(ES8388_CONTROL1, 0x12);  // Enfr=0,Play&Record Mode
    ES8388_Write(ES8388_DACCONTROL1, 0x18);  // 0x18:16bit iis
    ES8388_Write(ES8388_DACCONTROL2, 0x02);  // DACFsMode,SINGLE SPEED; DACFsRatio,256
    ES8388_Write(ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1
    ES8388_Write(ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
    ES8388_Write(ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
    ES8388_Write(ES8388_DACCONTROL21, 0x80); // set internal ADC and DAC use the same LRCK clock
    ES8388_Write(ES8388_DACCONTROL23, 0x00); // vroi=0

    ES8388_Write(ES8388_DACCONTROL4, 0x30);  // Left DAC digital volume 
    ES8388_Write(ES8388_DACCONTROL5, 0x30);  // Right DAC digital volume 

    
    // CRITICAL: Set ANALOG output volumes (DACCONTROL24-27 = 0x2E-0x31)
    // 0x00 = -96dB, 0x1E = 0dB, 0x21 = +3dB
    ES8388_Write(ES8388_DACCONTROL24, 0x1E); // LOUT1 volume 0dB
    ES8388_Write(ES8388_DACCONTROL25, 0x1E); // ROUT1 volume 0dB
    ES8388_Write(ES8388_DACCONTROL26, 0x00); // LOUT2 volume
    ES8388_Write(ES8388_DACCONTROL27, 0x00); // ROUT2 volume
    
    // Enable DAC and outputs (0x3C = all outputs)
    ES8388_Write(ES8388_DACPOWER, 0x3C);  // 0x3C Enable DAC and Enable Lout/Rout/1/2
    
    /* ADC */
    ES8388_Write(ES8388_ADCPOWER, 0xFF);
    ES8388_Write(ES8388_ADCCONTROL1, 0xbb);  // MIC Left and Right channel PGA gain
    ES8388_Write(ES8388_ADCCONTROL2, 0x00);  // LINSEL & RINSEL, LIN1/RIN1 as ADC Input
    ES8388_Write(ES8388_ADCCONTROL3, 0x02);
    ES8388_Write(ES8388_ADCCONTROL4, 0x0c);  // 16 Bits length and I2S serial audio data format
    ES8388_Write(ES8388_ADCCONTROL5, 0x02);  // ADCFsMode,singel SPEED,RATIO=256
    
    // Set ADC digital volume to 0dB
    ES8388_Write(ES8388_ADCCONTROL8, 0x00);
    ES8388_Write(ES8388_ADCCONTROL9, 0x00);
    
    ES8388_Write(ES8388_ADCPOWER, 0x09);  // Power on ADC, enable LIN&RIN
    
    UART_PrintLine("ES8388 Init Complete - Ready for Start");
}

esp_err_t es8388_start(es_module_t mode)
{
    esp_err_t res = ESP_OK;
    uint8_t prev_data = 0, data = 0;
    
    UART_PrintLine("-------------------------------------------");
    UART_PrintLine("es8388_start() - Starting DAC");
    UART_PrintLine("-------------------------------------------");
    
    prev_data = ES8388_Read(ES8388_DACCONTROL21);
    
    if (mode == ES_MODULE_LINE) {
        res |= ES8388_Write(ES8388_DACCONTROL16, 0x09);
        res |= ES8388_Write(ES8388_DACCONTROL17, 0x50);
        res |= ES8388_Write(ES8388_DACCONTROL20, 0x50);
        res |= ES8388_Write(ES8388_DACCONTROL21, 0xC0);
    } else {
        res |= ES8388_Write(ES8388_DACCONTROL21, 0x80);  // enable dac
    }
    
    data = ES8388_Read(ES8388_DACCONTROL21);
    
    if (prev_data != data) {
        UART_PrintLine("Triggering state machine...");
        res |= ES8388_Write(ES8388_CHIPPOWER, 0xF0);  // start state machine
        res |= ES8388_Write(ES8388_CHIPPOWER, 0x00);  // start state machine
        HAL_Delay(100);
    }
    
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        res |= ES8388_Write(ES8388_ADCPOWER, 0x00);  // power up adc and line in
    }
    
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        res |= ES8388_Write(ES8388_DACPOWER, 0x3C);  // power up dac and line out
        res |= ES8388_Write(ES8388_DACCONTROL3, 0x00);  // UNMUTE DAC
        UART_PrintLine("DAC powered up and UNMUTED");
    }
    
    UART_PrintLine("es8388_start() complete");
    UART_PrintLine("===========================================");
    
    return res;
}

esp_err_t es8388_config_dac_output(es_dac_output_t output)
{
    esp_err_t res = ESP_OK;
    uint8_t reg = ES8388_Read(ES8388_DACPOWER);
    reg = reg & 0xC3;
    res |= ES8388_Write(ES8388_DACPOWER, reg | output);
    return res;
}

esp_err_t es8388_set_voice_volume(int volume)
{
    esp_err_t res = ESP_OK;
    
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    // Convert 0-100 to register value
    // ESP32 uses DACCONTROL4/5 (0x1A/0x1B) for digital volume
    // 0xC0 = -96dB, 0x00 = 0dB
    // For simplicity, map 0-100 to 0xC0-0x00 (quietest to loudest)
    uint8_t reg_value = 0xC0 - ((volume * 0xC0) / 100);
    
    res |= ES8388_Write(ES8388_DACCONTROL5, reg_value);  // Right
    res |= ES8388_Write(ES8388_DACCONTROL4, reg_value);  // Left
    
    char buf[60];
    sprintf(buf, "Volume set to %d (reg=0x%02X)", volume, reg_value);
    UART_PrintLine(buf);
    
    return res;
}

esp_err_t es8388_stop(es_module_t mode)
{
    esp_err_t res = ESP_OK;
    
    if (mode == ES_MODULE_LINE) {
        res |= ES8388_Write(ES8388_DACCONTROL21, 0x80);
        res |= ES8388_Write(ES8388_DACCONTROL16, 0x00);
        res |= ES8388_Write(ES8388_DACCONTROL17, 0x90);
        res |= ES8388_Write(ES8388_DACCONTROL20, 0x90);
        return res;
    }
    
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8388_Write(ES8388_DACPOWER, 0x00);
        res |= ES8388_Write(ES8388_DACCONTROL3, 0x04);  // Mute
    }
    
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8388_Write(ES8388_ADCPOWER, 0xFF);
    }
    
    if (mode == ES_MODULE_ADC_DAC) {
        res |= ES8388_Write(ES8388_DACCONTROL21, 0x9C);
    }
    
    return res;
}