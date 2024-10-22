// src/components/LEDControl.js

import React, { useState, useEffect } from 'react';
import { View, TouchableOpacity, Text, Button, StyleSheet, Dimensions, Animated } from 'react-native';
import axios from 'axios';
import ColorPicker from 'react-native-wheel-color-picker'; // Instalamos este paquete para elegir color

export default function LEDControl() {
  const numRows = 8; // Número de filas
  const numCols = 8; // Número de columnas

  const [selectedColor, setSelectedColor] = useState('#ffffff'); // Color seleccionado
  const [message, setMessage] = useState(''); // Mensaje de estado
  const [isMessageVisible, setIsMessageVisible] = useState(false); // Controla la visibilidad del mensaje
  const [ledColors, setLedColors] = useState(
    Array(numRows).fill().map(() => Array(numCols).fill('#8a8888')) // Inicialmente todos los LEDs están apagados (color gris)
  );

  const screenWidth = Dimensions.get('window').width; // Ancho de la pantalla
  const screenHeight = Dimensions.get('window').height; // Alto de la pantalla

  // Función para actualizar el color del LED en una posición específica
  const updateLEDColor = async (x, y) => {
    const [r, g, b] = hexToRgb(selectedColor); // Convertir color hexadecimal a RGB

    try {
      const response = await axios.get('http://localhost:3000/set_pixel', {
        params: { x, y, r, g, b },
      });
      setMessage(`LED en (${x}, ${y}) actualizado a color ${selectedColor}`);
      setIsMessageVisible(true); // Hacer visible el mensaje

      // Actualizar el color localmente en la cuadrícula
      const updatedColors = [...ledColors];
      updatedColors[x][y] = selectedColor;
      setLedColors(updatedColors);

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => {
        setIsMessageVisible(false);
      }, 3000); // Cambia el tiempo en milisegundos si es necesario

    } catch (error) {
      console.error('Error al actualizar el LED', error);
      setMessage('Error al actualizar el LED');
      setIsMessageVisible(true); // Hacer visible el mensaje

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => {
        setIsMessageVisible(false);
      }, 3000); // Cambia el tiempo en milisegundos si es necesario
    }
  };

  // Función para activar el modo dinámico
  const activateDynamicMode = async () => {
    try {
      const response = await axios.get('http://192.168.4.1/modo_dinamico');
      setIsMessageVisible(true); // Hacer visible el mensaje

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => {
        setIsMessageVisible(false);
      }, 3000); // Cambia el tiempo en milisegundos si es necesario
    } catch (error) {
      console.error('Error al activar el modo dinámico', error);
      setMessage('Error al activar el modo dinámico');
      setIsMessageVisible(true); // Hacer visible el mensaje

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => {
        setIsMessageVisible(false);
      }, 3000); // Cambia el tiempo en milisegundos si es necesario
    }
  };

  // Función para convertir un valor hexadecimal a RGB
  const hexToRgb = (hex) => {
    const bigint = parseInt(hex.slice(1), 16);
    const r = (bigint >> 16) & 255;
    const g = (bigint >> 8) & 255;
    const b = bigint & 255;
    return [r, g, b];
  };

  // Calcular el tamaño del LED en función del ancho de la pantalla
  const ledSize = Math.min(screenWidth, screenHeight) / (Math.max(numRows, numCols) + 2);

  return (
    <View style={styles.container}>
      {/* Color picker para seleccionar un color */}
      <ColorPicker
        color={selectedColor}
        onColorChange={(color) => setSelectedColor(color)}
        thumbSize={30}
        sliderSize={20}
        noSnap={true}
        row={false}
      />

      {/* Cuadrícula de LEDs */}
      <View style={styles.grid}>
        {Array.from({ length: numRows }).map((_, rowIndex) => (
          <View key={rowIndex} style={styles.row}>
            {Array.from({ length: numCols }).map((_, colIndex) => (
              <TouchableOpacity
                key={colIndex}
                style={[
                  styles.led,
                  {
                    backgroundColor: ledColors[rowIndex][colIndex],
                    width: ledSize,
                    height: ledSize,
                  },
                ]}
                onPress={() => updateLEDColor(rowIndex, colIndex)} // Actualiza el LED cuando se pulsa
              />
            ))}
          </View>
        ))}
      </View>

      {/* Botón para activar modo dinámico */}
      <Button title="Activar Modo Dinámico" onPress={activateDynamicMode} />

      {/* Mensaje de estado como pop-up */}
      {isMessageVisible && (
        <Text style={styles.message}>{message}</Text>
      )}
    </View>
  );
}

// Estilos
const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#424242',
    padding: 10,
    width: '100%',
  },
  grid: {
    marginVertical: 20,
  },
  row: {
    flexDirection: 'row',
  },
  led: {
    margin: 2,
    borderRadius: 10,
    borderWidth: 4,
    borderColor: '#3d3d3d',
  },
  message: {
    marginTop: 15,
    fontSize: 18,
    color: 'red',
    textAlign: 'center',
    position: 'absolute', // Para que el mensaje aparezca sobre otros componentes
    top: 170, // Puedes ajustar esto según sea necesario
    padding: 10,
    borderRadius: 5,
    elevation: 2, // Sombra en Android
    shadowColor: '#000', // Sombra en iOS
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.2,
    shadowRadius: 1,
  },
});
