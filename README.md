#### ITBA - Laboratorio de Microprocesadores - 2018

# IMU-con-FRDM-K64
ITBA - Laboratorio de Microprocesadores - 2018 - Implementación de una unidad de medición inercial (IMU) utilizando la placa de desarrollo FRDM-K64, la cual posee un acelerómetro/magnetómetro (FXOS8700CQ). El programa grafica en tiempo real el movimiento de todas las placas que se encuentren conectadas entre si.  Se sensa la posición de la placa (utilizado I2C para comunicarse con el acelerometro), se comunica la misma y se obtienen las mediciones de las otras placas utilizando un bus CAN, y finalmente se comunican los datos a la interfaz gráfica en la computadora utilizando UART.
