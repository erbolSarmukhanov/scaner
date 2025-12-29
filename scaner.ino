#include <SPI.h>
#include <mcp2515.h>

struct can_frame can_tx;
struct can_frame can_rx;
MCP2515 mcp2515(7);

void setup() {
  Serial.begin(115200);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("--- CAN VIN Reader Ready ---");
  Serial.println("Отправка запроса Service 09 PID 02...");

  // 1. Формируем запрос VIN
  can_tx.can_id  = 0x7E0; // ID двигателя (стандарт)
  can_tx.can_dlc = 8;
  can_tx.data[0] = 0x02; // Длина данных (02)
  can_tx.data[1] = 0x09; // Сервис 09
  can_tx.data[2] = 0x04; // Инфо-тип 02 (VIN)
  can_tx.data[3] = 0x00;
  can_tx.data[4] = 0x00;
  can_tx.data[5] = 0x00;
  can_tx.data[6] = 0x00;
  can_tx.data[7] = 0x00;

  mcp2515.sendMessage(&can_tx);
}

void loop() {
  if (mcp2515.readMessage(&can_rx) == MCP2515::ERROR_OK) {
    if (can_rx.can_id == 0x7E8) {
      
      // Начало сообщения (First Frame)
      if (can_rx.data[0] == 0x10) {
        Serial.println();
        Serial.print("Data Found: ");
        
        // В 09 сервисе полезные данные в первом кадре начинаются с 5-го байта
        for (int i = 5; i < 8; i++) {
          if (can_rx.data[i] > 31 && can_rx.data[i] < 127) Serial.print((char)can_rx.data[i]);
        }

        // Отправляем Flow Control
        struct can_frame fc;
        fc.can_id = 0x7E0; fc.can_dlc = 8;
        fc.data[0] = 0x30; fc.data[1] = 0x00; fc.data[2] = 0x00;
        mcp2515.sendMessage(&fc);
      } 
      // Продолжение (Consecutive Frames)
      else if ((can_rx.data[0] & 0xF0) == 0x20) {
        for (int i = 1; i < 8; i++) {
          if (can_rx.data[i] > 31 && can_rx.data[i] < 127) Serial.print((char)can_rx.data[i]);
        }
      }
    }
  }
}