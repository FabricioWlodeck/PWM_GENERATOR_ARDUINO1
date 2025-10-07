/* 
Materia: Sistemas Embebidos
Author: Wlodeck Fabricio Joaquin - 30/09/2025
Grupo: Dutra-Wlodeck

*/
#ifndef F_CPU
    #define F_CPU  16000000UL // Define la macro SOLO si no ha sido definida antes.
#endif
//------------------------------------------------------------

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>

/* 
Incremento segun Tiempo seleccionado:
- teniendo en cuenta 100 Hz y 100 DC por seg
- T1=6 seg -> hay 600 ciclos PWM
- T1=10 seg -> hay 1000 ciclos PWM
- T1=15 seg -> hay 1500 ciclos PWM

------- Crecimiento de DC (1 unidad de DC = 1% )-------
- T1 -> 600 ciclos / 100 unidades = 1 % cada 6 ciclos o 60 ms
- T2 -> 1000 ciclos / 100 unidades = 1 % cada 10 ciclos o 100 ms
- T3 -> 1500 ciclos / 100 unidades = 1 % cada 15 ciclo o 150 ms 

------- Secuencia de LEDS -------
- T1 -> Prende 1 led cada 6000 ms/5 = 1200 ms
- T2 -> Prende 1 led cada 10000 ms/5 = 2000 ms
- T3 -> Prende 1 led cada 15000 ms/5 = 3000 ms

*/

// --- DEFINICIONES DE PINES ---
#define PWM_OUT PB0 // Salida de la señal PWM simulada
#define DEBOUNCE_DELAY  10  // Retardo para eliminar el rebote 10 ms

// Pulsadores (Polling)
/* #define PD3_CHANGE_DC   PD3     // pulsador para cambiar arranque */
#define PD7_OFF_SOFTBOOT   PD7		// pulsador para detener arranque suave


// constantes
#define t1_incremento 60         //cada cuanto incrementa en ms
#define t2_incremento 100        //cada cuanto incrementa en ms
#define t3_incremento 150        //cada cuanto incrementa en ms

/* 
// Pulsador (interrupción INTO)
#define PD2_ON_OFF  PD2     // pulsador para encender/apagar el motor
 */
// ------- Macros -------
#define is_high(p,b)	(p & _BV(b)) == _BV(b)    // devuelve True o 1 si el bit b de p es 1.
#define is_low(p,b)		(p & _BV(b)) == 0         // devuelve True o 1 si el bit b de p es 0.
#define sbi(p,b)		p |= _BV(b)                // sbi(p,b) setea el bit b de p.
#define cbi(p,b)		p &= ~(_BV(b))             // cbi(p,b) borra el bit b de p.
#define tbi(p,b)		p ^= _BV(b)                // tbi(p,b) togglea el bit b de p.

volatile uint8_t last_state_P3 = 0;
// Flags botones
volatile uint8_t on_flag = 0;
volatile uint8_t change_t_flag = 0; //0: 6s, 1: 10s y 2: 15s
volatile uint8_t off_soft_flag = 0;


volatile uint16_t soft_time_ms = 6000; // Arranque suave inicial 
volatile uint8_t duty_t = 0;
volatile uint8_t duty_upgrate_t = t1_incremento;
volatile uint8_t duty_steps = 0;

volatile uint8_t operation_point = 0;
volatile uint8_t ramp_time_progress_ms = 0;

volatile uint8_t time_passed = 0;
uint8_t chosen_time = 1;
uint8_t change_time = 0;


// Funcion que controla la seleccion y avance del pwm
void pwm_generator(uint16_t time_start, uint16_t duty_cycle);

void delay_time_generator(uint32_t us);


//------------------------------------------------------------

// manejo interrupcion externa con INT-0
ISR(INT0_vect) {
    // Rutina de interrupción externa INT0
    if(on_flag){
      chosen_time = 1; 
    }
    on_flag = !on_flag;
    
    duty_t = 0;
    time_passed = 0;
    duty_upgrate_t = 0;
    ramp_time_progress_ms = 0;
    _delay_ms(DEBOUNCE_DELAY); // Antirebote simple, no recomendado. Arreglo despues lo juro
}

ISR(INT1_vect) {
  // Rutina de interrupción externa INT1
  // Rota entre las opciones de timempo T
  change_t_flag = 1; //0: No hacer cambio, 1: hacer cambio
  chosen_time = 1;
  _delay_ms(DEBOUNCE_DELAY); // Antirebote simple, no recomendado. Arreglo despues lo juro
}

int main(void) {
  /*  */
  DDRB = 0b111111; //define todos los puertos B (son 6)como salida;
  /* PORTB = 0x00; // puerto B inicializados en 0 */
  DDRD = 0x00;  // Declaro los puertos PDn como entradas

  // === Configuración de interrupción externa INT0 y INT1 ===
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);
  EIMSK |= (1 << INT0);
  EIMSK |= (1 << INT1);
  // Habilitar interrupciones globales
  sei();
  duty_t = 1;

  // ----------- Secuencia de Inicio --------------------
  PORTB |= (1 << PORTB5);
  PORTB |= (1 << PORTB4);
  PORTB |= (1 << PORTB3);
  PORTB |= (1 << PORTB2);
  PORTB |= (1 << PORTB1);
  PORTB |= (1 << PORTB0);
  _delay_ms(250);
  PORTB &= ~(0 << PORTB5);
  PORTB &= ~(0 << PORTB4);
  PORTB &= ~(0 << PORTB3);
  PORTB &= ~(0 << PORTB2);
  PORTB &= ~(0 << PORTB1);
  PORTB &= ~(0 << PORTB0);
  _delay_us(250000);

  

  while(1){
    if(change_t_flag){ 
      switch(soft_time_ms){
        case 6000:
          soft_time_ms = 10000;
        break;

        case 10000:
          soft_time_ms = 15000;
        break;

        case 15000:
          soft_time_ms = 6000;
        break;
      }
      change_t_flag = 0;
      change_time = 0;
    }
    
    if(chosen_time && on_flag){
      cbi(PORTB, PORTB5); 
      cbi(PORTB, PORTB4);
      cbi(PORTB, PORTB3);
      cbi(PORTB, PORTB2);
      cbi(PORTB, PORTB1);
      switch(soft_time_ms){
        case 6000:
          sbi(PORTB, PORTB5); 
        break;
        case 10000:
          sbi(PORTB, PORTB4);
        break;

        case 15000:
          sbi(PORTB, PORTB3);
        break;
      }
      delay_time_generator(500000); 
      cbi(PORTB, PORTB5);
      cbi(PORTB, PORTB4);
      cbi(PORTB, PORTB3);
      cbi(PORTB, PORTB2);
      cbi(PORTB, PORTB1);
      chosen_time = 0;
    }
    

    if(on_flag){
      switch(soft_time_ms){
        case 6000:
        duty_upgrate_t = t1_incremento;
        duty_steps=6;
        break;

        case 10000:
        duty_upgrate_t = t2_incremento;
        duty_steps=10;
        break;

        case 15000:
        duty_upgrate_t = t3_incremento;
        duty_steps=15;
        break;
      }


      while(on_flag){
        for(duty_t; duty_t <= 99; duty_t++){
        for(ramp_time_progress_ms=1; ramp_time_progress_ms < duty_steps; ramp_time_progress_ms++){
          pwm_generator(soft_time_ms, duty_t);
        }
        // ----------- Apagado arraque suave --------------------
        if (is_low(PIND, PD7_OFF_SOFTBOOT)) {   //comprueba Boton 3
            _delay_ms(DEBOUNCE_DELAY);  // esperar para evitar el rebote
            if (is_low(PIND, PD7_OFF_SOFTBOOT) && last_state_P3 == 0) { //si sigue bajo y el ultimo estado no estaba seteado
              if(on_flag){
                chosen_time = 1; 
                /* change_t_flag = 1; */
              }
              on_flag = !on_flag; // toglear la bandera del boton
              last_state_P3 = 1;   // actualizar el ultimo estado
            }
        }else { // restablecer el estado cuando el boton no se esta presionando
          last_state_P3 = 0;  
        }
         if (!on_flag) { 
            PORTB = 0b000000;
            break; // Sale inmediatamente del bucle de incremento de DC (tiempo de rampa)
          } 
        }
      
        pwm_generator(soft_time_ms, duty_t); // mantener PWM al 100%
      }
      
    } else{ // si el pulsador esta off:
      PORTB = 0b000000;
      duty_t = 0;
      time_passed = 0;
      duty_upgrate_t = 0;
      ramp_time_progress_ms = 0;
    }
  };

};

void delay_time_generator(uint32_t us){
  while(us--){
    _delay_us(1);
  }
}
// frecuencia 100Hz -> Periodo 10.000 us
// 1% de 10.000 us es un aumento de 100 us de Duty Cycle
void pwm_generator(uint16_t time_start, uint16_t duty_cycle){
  //suponiendo que el duty_cycle empieza en "1"

  if(duty_cycle < 20){
    // Todos los leds apagados
    PORTB &= ~(0 << PORTB5);
    PORTB &= ~(0 << PORTB4);
    PORTB &= ~(0 << PORTB3);
    PORTB &= ~(0 << PORTB2);
    PORTB &= ~(0 << PORTB1);
  } else if (duty_cycle >= 20 && duty_cycle < 40) {
    //LED 1 prendido
    PORTB |= (1 << PORTB5);
    PORTB &= ~(0 << PORTB4);
    PORTB &= ~(0 << PORTB3);
    PORTB &= ~(0 << PORTB2);
    PORTB &= ~(0 << PORTB1);

  } else if (duty_cycle >= 40 && duty_cycle < 60) {
    //LEDS del 1 al 2 prendidos
    PORTB |= (1 << PORTB5);
    PORTB |= (1 << PORTB4);
    PORTB &= ~(0 << PORTB3);
    PORTB &= ~(0 << PORTB2);
    PORTB &= ~(0 << PORTB1);

  } else if (duty_cycle >= 60 && duty_cycle < 80) {
    //LEDS del 1 al 3 prendidos
    PORTB |= (1 << PORTB5);
    PORTB |= (1 << PORTB4);
    PORTB |= (1 << PORTB3);
    PORTB &= ~(0 << PORTB2);
    PORTB &= ~(0 << PORTB1);

  } else if (duty_cycle >= 80 && duty_cycle < 100) {
    //LEDS del 1 al 4 prendidos
    PORTB |= (1 << PORTB5);
    PORTB |= (1 << PORTB4);
    PORTB |= (1 << PORTB3);
    PORTB |= (1 << PORTB2);
    PORTB &= ~(0 << PORTB1);

  } else if(duty_cycle == 100){
    //Todos prendido REGIMEN PERMANENTE
    PORTB |= (1 << PORTB5);
    PORTB |= (1 << PORTB4);
    PORTB |= (1 << PORTB3);
    PORTB |= (1 << PORTB2);
    PORTB |= (1 << PORTB1);

  }else if(duty_cycle > 100){
    //De esta manera estoy detectando errores de manera visual actualmente
    PORTB |= (1 << PORTB5);
    PORTB &= ~(0 << PORTB4);
    PORTB |= (1 << PORTB3);
    PORTB &= ~(0 << PORTB2);
    PORTB |= (1 << PORTB1);

    delay_time_generator(10000000);
  }

  uint32_t periode_t_us = 10000; //10000 us para una frecuencia de 100
  uint32_t time_on = (periode_t_us/100) * duty_cycle;   //100 us x (% duty time)
  uint32_t time_off = periode_t_us - time_on; //99%


  if(time_on>0){
    // tiempo en alto
    sbi(PORTB, PWM_OUT); // Activar el motor
    delay_time_generator(time_on);
  }
	
  if(time_off>0){
    //tiempo en bajo
    cbi(PORTB, PWM_OUT); // Apagar el motor
    delay_time_generator(time_off);
  }
	
}


