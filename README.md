# tca9535_mux

TCA9535 I2C multiplexer output only.

Экспорт в `/sys/devices/platform/tca9535-mux.n`:

* `address` - I2C адрес на шине
* `input` - значение input регистров
* `out` - значение output регистров
* `config` - значение config регистров
* `polarity` - значение polarity регистров

Все значения регистов 16 битные. Внутри микросхемы регистры 8 бит, модуль ядра их объединяет в 16 бит по (high << 8) + low.

Пример devicetree для 4х мультиплексоров:

```
/dts-v1/;
/plugin/;
/ {
    fragment@0 {
		target = <&i2c1>;
        
        __overlay__ {
            tca9535_mux@20 {
                compatible = "ti,tca9535-mux";
                reg = <0x20>;
                config-mask = <0x0000>;     // All outputs
                polarity-mask = <0x0000>;   // No polarity inversion
                output-default-mask = <0xFFFF>;
                
                status = "okay";
            };
            
            tca9535_mux@24 {
                compatible = "ti,tca9535-mux";
                reg = <0x24>;
                config-mask = <0x0000>;     // All outputs
                polarity-mask = <0x0000>;   // No polarity inversion
                output-default-mask = <0xFFFF>;

                status = "okay";
            };

            tca9535_mux@26 {
                compatible = "ti,tca9535-mux";
                reg = <0x26>;
                config-mask = <0x0000>;     // All outputs
                polarity-mask = <0x0000>;   // No polarity inversion
                output-default-mask = <0xFFFF>;

                status = "okay";
            };

            tca9535_mux@27 {
                compatible = "ti,tca9535-mux";
                reg = <0x27>;
                config-mask = <0x0000>;     // All outputs
                polarity-mask = <0x0000>;   // No polarity inversion
                output-default-mask = <0xFFFF>;

                status = "okay";
            };
        };
    };
};
```
