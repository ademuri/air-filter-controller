## Design decisions

### Power supply scheme

The ESP32-S3 uses 3.3V. I want onboard indicator LEDs, and the option to have offboard LEDs as well. Using addressable WS2812-type LEDs is flexible. These need a 5V supply. The fans draw 12V, so this necessitates two voltage regulators.

One option would be to use separate buck converters for both the 5V and 3.3V rails.

With no offboard LEDs, the 5V rail power draw will be low. WS2812B LEDs consume roughly 1mA each when idle. When lit at a fully-saturated color, they will consume about 20mA each. There are four onboard LEDs, so this means 4mA idle, and 80mA when lit.

This is relatively low power consumption. However, offboard LEDs may need significantly more power. If, for example, a 16-LED ring is used, then up to 400mA total may be needed.

During normal operation, the indicator light will be off. So, most of the time, the 5V current draw will be low.

To convert from the 12V supply to 5V, we'll use a buck converter. If we size the inductor for the maximum current draw, then the converter will potentially be noisy during typical operation.

So, we should instead use a linear regulator to convert the 5V to 3.3V, if possible. This will simplify the design, and increase the minimum current draw on the buck converter. Is it feasible?

The ESP32-S3 datasheet says that the power supply should support at least 500mA. The AMS1117-3.3 supports 1A max. The typical power draw of the ESP32-S3 running our ESPHome configuration is 30-120mA. A linear regulator burns the excess voltage as heat. For (worst-case) typical operation, the power consumed by the regulator will be `.12A * (5V - 3.3V) = 200mW`. This is acceptable.

Using [this buck converter calculator](http://schmidt-walter-schaltnetzteile.de/smps_e/abw_smps_e.html), a 100uH inductor means that the maximum ripple current will be about 50mA.

The chosen inductor (`SWPA8065S101MT`) has a max DC resistance of 0.28Ω, and a max heat-rating current of 1.35A. If the ESP32-S3 draws 500mA, this leaves >500mA for LEDs. This is sufficient.
