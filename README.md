# ESP32 Water Quality Sensor with External ADC

Este proyecto implementa un sensor de calidad de agua usando ESP32 con un ADC externo ADS1115 para medición precisa de conductividad eléctrica y compensación automática por temperatura.

## Características Principales

- **Medición de Conductividad Eléctrica**: Sensor TDS con compensación automática por temperatura
- **ADC Externo ADS1115**: Para lecturas precisas de voltajes pequeños
- **Sensor de Temperatura**: DS18B20 para compensación térmica
- **Calibración Automática**: Ajuste del valor K a través de interfaz web
- **Pantalla LCD**: Visualización local de datos en tiempo real
- **Punto de Acceso WiFi**: Configuración y calibración remota
- **Memoria Flash**: Almacenamiento permanente de parámetros de calibración

## ¿Por qué usar un ADC Externo?

El ESP32 incluye un ADC interno, sin embargo, para este proyecto se utiliza el ADS1115 por las siguientes razones críticas:

### Problemas del ADC interno del ESP32:
1. **No linealidad**: El ADC interno del ESP32 presenta respuesta no lineal, especialmente en los extremos del rango
2. **Baja precisión en voltajes pequeños**: No puede leer con precisión voltajes pequeños (< 0.1V)
3. **Ruido**: Mayor susceptibilidad al ruido eléctrico
4. **Resolución limitada**: 12 bits efectivos vs 16 bits del ADS1115

### Ventajas del ADS1115:
- **16 bits de resolución real**: Mayor precisión en las mediciones
- **Respuesta completamente lineal**: Lecturas consistentes en todo el rango
- **Amplificador programable**: Puede amplificar señales pequeñas automáticamente
- **Bajo ruido**: Diseñado específicamente para mediciones de precisión
- **Múltiples canales**: Hasta 4 entradas analógicas independientes

## Componentes Requeridos

### Hardware
- ESP32 DevKit
- ADS1115 (ADC externo de 16 bits)
- Sensor TDS/conductividad eléctrica
- Sensor de temperatura DS18B20
- LCD 16x2 con interfaz I2C
- Resistencia pull-up 4.7kΩ (para DS18B20)
- Protoboard y cables de conexión

### Software
- Arduino IDE
- Librerías requeridas (ver sección de instalación)

## Conexiones

```
ESP32          ADS1115
VCC      <->   VDD
GND      <->   GND
GPIO21   <->   SDA
GPIO22   <->   SCL

ESP32          DS18B20
GPIO4    <->   Data (con resistencia pull-up 4.7kΩ a VCC)
VCC      <->   VDD
GND      <->   GND

ESP32          LCD I2C
GPIO21   <->   SDA
GPIO22   <->   SCL
VCC      <->   VDD
GND      <->   GND

ADS1115        Sensor TDS
A0       <->   Salida analógica del sensor
```

## Instalación de Librerías

En Arduino IDE, instala las siguientes librerías:

```cpp
#include <Wire.h>                // Comunicación I2C (incluida en Arduino)
#include <LiquidCrystal_I2C.h>   // Manejo del LCD I2C
#include <OneWire.h>             // Protocolo OneWire para DS18B20
#include <DallasTemperature.h>   // Sensor de temperatura DS18B20
#include <WiFi.h>                // WiFi para ESP32 (incluida)
#include <Preferences.h>         // Memoria flash ESP32 (incluida)
#include <Adafruit_ADS1X15.h>    // ADC externo ADS1115
```

## Configuración

### 1. WiFi
Modifica las credenciales del punto de acceso en el código:
```cpp
const char* ssid     = "EMA.AGUA";      // Nombre de la red
const char* password = "EMA.AGUA";      // Contraseña
```

### 2. Calibración
1. Conecta a la red WiFi "EMA.AGUA"
2. Abre un navegador web y ve a la IP mostrada en el LCD
3. Introduce un valor conocido de conductividad para calibrar el sensor

## Funcionamiento

### Medición de Conductividad
El sistema mide la conductividad eléctrica del agua usando:
1. **Lectura del ADC**: El ADS1115 lee el voltaje del sensor TDS
2. **Filtro de mediana**: Reduce el ruido en las mediciones
3. **Compensación por temperatura**: Ajusta la lectura según la temperatura actual
4. **Cálculo final**: Aplica la fórmula de conversión con el factor K calibrado

### Fórmula de Conversión
```
TDS = (133.42 × V³ - 255.86 × V² + 857.39 × V) × 0.5 × K
```
Donde:
- V = Voltaje compensado por temperatura
- K = Factor de calibración

### Compensación por Temperatura
```
Coeficiente = 1.0 + 0.02 × (Temperatura - 25.0)
Voltaje_compensado = Voltaje_medido / Coeficiente
```

## Interfaz Web

El dispositivo crea un punto de acceso WiFi que permite:
- Ver el valor actual de conductividad
- Mostrar el factor K de calibración
- Introducir valores conocidos para recalibrar el sensor
- Monitorear el estado del sistema

## Pantalla LCD

Muestra en tiempo real:
- **Línea 1**: Dirección IP del punto de acceso
- **Línea 2**: Conductividad eléctrica (CE) en µS/cm y temperatura en °C

## Características Técnicas

### Precisión del Sistema
- **Resolución ADC**: 16 bits (65,535 niveles)
- **Rango de voltaje**: 0-6.144V (configurable)
- **Precisión temperatura**: ±0.5°C
- **Tiempo de respuesta**: ~500ms por muestra

### Almacenamiento
- **Memoria flash**: Guarda el factor K de calibración permanentemente
- **Recuperación automática**: Carga la calibración al reiniciar

## Troubleshooting

### Problemas Comunes

1. **LCD no muestra nada**
   - Verificar conexiones I2C
   - Confirmar dirección I2C del LCD (0x27)

2. **Error "Failed to initialize ADS"**
   - Verificar conexiones I2C del ADS1115
   - Confirmar dirección I2C (0x48)

3. **Lecturas inestables**
   - Verificar conexiones del sensor TDS
   - Limpiar electrodos del sensor
   - Verificar calibración

4. **No se conecta al WiFi**
   - Verificar que el ESP32 esté en modo AP
   - Buscar red "EMA.AGUA"

## Contribuciones

Este proyecto fue desarrollado para medición precisa de calidad de agua con compensación automática por temperatura. Las mejoras y sugerencias son bienvenidas.

## Licencia

Este proyecto es de código abierto y está disponible bajo licencia MIT.

---

**Nota**: El uso del ADC externo ADS1115 es fundamental para obtener mediciones precisas de conductividad eléctrica, ya que los valores de voltaje del sensor TDS son muy pequeños y requieren alta linealidad para cálculos exactos.
