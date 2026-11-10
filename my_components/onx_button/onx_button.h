#pragma once
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "freertos/FreeRTOS.h"

namespace esphome::onx_button {
// Display transfers can occupy a loop longer than a human click. Capture both
// edges in the ISR; run debounce and automations only on the ESPHome task.
class BufferedButton : public binary_sensor::BinarySensor, public Component {
 public:
  void set_pin(InternalGPIOPin *pin) {pin_=pin;}
  uint32_t event_time_ms() const {return event_ms_;}
  bool is_physically_pressed() const {return pin_->digital_read();}
  float get_setup_priority() const override {return setup_priority::HARDWARE;}
  void setup() override {
    pin_->setup();
    isr_pin_=pin_->to_isr();
    accepted_=pin_->digital_read();
    event_ms_=millis();
    publish_initial_state(accepted_);
    pin_->attach_interrupt(&BufferedButton::interrupt_,this,gpio::INTERRUPT_ANY_EDGE);
  }
  void loop() override {
    for(;;) {
      Edge edge{},next{};
      bool has_next,overflow;
      portENTER_CRITICAL(&lock_);
      overflow=overflow_;
      if(overflow) {head_=tail_;overflow_=false;}
      const uint8_t count=(head_-tail_)&MASK;
      if(count) {
        edge=edges_[tail_];
        has_next=count>1;
        if(has_next)next=edges_[(tail_+1)&MASK];
      } else has_next=false;
      portEXIT_CRITICAL(&lock_);
      if(overflow) {
        // Never turn a partial, overflowing edge sequence into a click or hold.
        recovering_=true;
        ESP_LOGW("onx_button","Button edge queue overflow; waiting for release");
      }
      if(recovering_) {
        if(!pin_->digital_read()) {
          recovering_=false;accepted_=false;event_ms_=millis();
          publish_initial_state(false);
        }
        return;
      }
      if(!count)return;
      const uint32_t stable_ms=(has_next?next.ms:millis())-edge.ms;
      if(!has_next && stable_ms<5)return;
      portENTER_CRITICAL(&lock_);
      tail_=(tail_+1)&MASK;
      portEXIT_CRITICAL(&lock_);
      if(stable_ms<5 || edge.state==accepted_)continue;
      accepted_=edge.state;
      event_ms_=edge.ms;
      publish_state(edge.state);
    }
  }
 protected:
  struct Edge {uint32_t ms;bool state;};
  static constexpr uint8_t MASK=63;
  static void IRAM_ATTR interrupt_(BufferedButton *self) {
    const Edge edge{millis(),self->isr_pin_.digital_read()};
    portENTER_CRITICAL_ISR(&self->lock_);
    const uint8_t next=(self->head_+1)&MASK;
    if(next==self->tail_)self->overflow_=true;
    else {self->edges_[self->head_]=edge;self->head_=next;}
    portEXIT_CRITICAL_ISR(&self->lock_);
  }
  InternalGPIOPin *pin_{nullptr};
  ISRInternalGPIOPin isr_pin_;
  portMUX_TYPE lock_=portMUX_INITIALIZER_UNLOCKED;
  Edge edges_[64]{};
  uint8_t head_{0},tail_{0};
  bool accepted_{false},overflow_{false},recovering_{false};
  uint32_t event_ms_{0};
};
}
