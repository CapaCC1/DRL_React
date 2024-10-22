import React, { useState, useEffect } from 'react';
import { View, TouchableOpacity, Text, Button, StyleSheet, Dimensions } from 'react-native';
import axios from 'axios';
import ColorPicker from 'react-native-wheel-color-picker';

export default function LEDControl() {
  const numRows = 8; // Número de filas
  const numCols = 8; // Número de columnas

  const [selectedColor, setSelectedColor] = useState('#ffffff');
  const [message, setMessage] = useState('');
  const [isMessageVisible, setIsMessageVisible] = useState(false);
  const [ledColors, setLedColors] = useState(Array(numRows).fill().map(() => Array(numCols).fill('#8a8888')));

  const screenWidth = Dimensions.get('window').width;
  const screenHeight = Dimensions.get('window').height;

  // Función para actualizar el color del LED en una posición específica
  const updateLEDColor = async (x, y) => {
    const [r, g, b] = hexToRgb(selectedColor);

    try {
      await axios.get('http://192.168.1.144/set_pixel', {
        params: { x, y, r, g, b },
      });

      // Actualizar el color localmente en la cuadrícula
      const updatedColors = [...ledColors];
      updatedColors[y][x] = selectedColor; // Cambié (x, y) a (y, x) para que coincida con el índice de filas y columnas
      setLedColors(updatedColors);

      setMessage(`LED en (${x}, ${y}) actualizado a color ${selectedColor}`);
      setIsMessageVisible(true);

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => setIsMessageVisible(false), 3000);

    } catch (error) {
      console.error('Error al actualizar el LED', error);
      setMessage('Error al actualizar el LED');
      setIsMessageVisible(true);
    }
  };

  // Función para obtener el estado actual de los LEDs
  const fetchLEDStatus = async () => {
    try {
      const response = await axios.get('http://192.168.1.144/led_status');
      const colors = response.data.map(led => `#${((1 << 24) + (led.r << 16) + (led.g << 8) + led.b).toString(16).slice(1)}`);
      const updatedColors = Array(numRows).fill().map(() => Array(numCols).fill('#8a8888'));

      // Llenar la cuadrícula con los colores obtenidos
      for (let i = 0; i < colors.length; i++) {
        const row = Math.floor(i / numCols);
        const col = i % numCols;
        updatedColors[row][col] = colors[i];
      }

      setLedColors(updatedColors);
    } catch (error) {
      console.error('Error al obtener el estado de los LEDs', error);
    }
  };

  // Usar un efecto para consultar el estado de los LEDs cada segundo
  useEffect(() => {
    const interval = setInterval(fetchLEDStatus, 50); // Cada 1000 ms = 1 segundo
    return () => clearInterval(interval); // Limpiar el intervalo al desmontar el componente
  }, []);

  // Función para activar el modo dinámico
  const activateDynamicMode = async () => {
    try {
      await axios.get('http://192.168.1.144/modo_dinamico');
      setMessage('Modo dinámico activado');
      setIsMessageVisible(true);

      // Ocultar el mensaje después de 3 segundos
      setTimeout(() => setIsMessageVisible(false), 3000);
    } catch (error) {
      console.error('Error al activar el modo dinámico', error);
      setMessage('Error al activar el modo dinámico');
      setIsMessageVisible(true);
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

  const ledSize = Math.min(screenWidth, screenHeight) / (Math.max(numRows, numCols) + 2);

  return (
    <View style={styles.container}>
      <ColorPicker
        color={selectedColor}
        onColorChange={(color) => setSelectedColor(color)}
        thumbSize={30}
        sliderSize={20}
        noSnap={true}
        row={false}
      />
      <View style={styles.grid}>
        {Array.from({ length: numRows }).map((_, rowIndex) => (
          <View key={rowIndex} style={styles.row}>
            {Array.from({ length: numCols }).map((_, colIndex) => (
              <TouchableOpacity
                key={colIndex}
                style={[styles.led, { backgroundColor: ledColors[rowIndex][colIndex], width: ledSize, height: ledSize }]}
                onPress={() => updateLEDColor(colIndex, rowIndex)} // Cambié (rowIndex, colIndex) a (colIndex, rowIndex)
              />
            ))}
          </View>
        ))}
      </View>
      <Button title="Activar Modo Dinámico" onPress={activateDynamicMode} />
      {isMessageVisible && <Text style={styles.message}>{message}</Text>}
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
    position: 'absolute',
    top: 170,
    padding: 10,
    borderRadius: 5,
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.2,
    shadowRadius: 1,
  },
});
//e