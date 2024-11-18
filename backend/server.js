const express = require('express');
const axios = require('axios');
const path = require('path');
const cors = require('cors'); // Importa cors

const app = express();
app.use(cors());

// Configura la dirección IP del ESP32 (ajusta según tu red)
const ESP32_IP = 'http://192.168.1.143'; // Cambia por la IP del ESP32 en tu red local

// Middleware para servir archivos estáticos
app.use(express.static('views'));

// Ruta para cambiar el color de un píxel
app.get('/set_pixel', async (req, res) => {
  const { x, y, r, g, b } = req.query;

  try {
    // Enviar una petición GET al ESP32 para actualizar el color del LED
    await axios.get(`${ESP32_IP}/set_pixel`, {
      params: { x, y, r, g, b }
    });
    res.send('LED actualizado');
  } catch (error) {
    console.error(error);
    res.status(500).send('Error al controlar el LED');
  }
});

// Ruta para mostrar el frontend
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'views', 'index.html'));
});


// Ruta para activar el modo dinámico
app.get('/modo_show_1', async (req, res) => {
  try {
    // Hacer una petición al ESP32 para activar el modo dinámico
    await axios.get(`${ESP32_IP}/modo_show_1`);
    console.log("Modo show activado")
    res.send('Modo show activado');
  } catch (error) {
    console.log(error.message);
    res.status(500).send('Error al activar el modo show');
  }
});

// Ruta para desactivar el modo dinámico
app.get('/desactivar-modo-show_1', async (req, res) => {
  try {
    // Hacer una petición al ESP32 para desactivar el modo dinámico
    await axios.get(`${ESP32_IP}/desactivar-modo-show-1`);
    console.log("Modo show desactivado");
    res.send('Modo show desactivado');
  } catch (error) {
    console.log(error.message);
    res.status(500).send('Error al desactivar el modo show');
  }
});

// Ruta para activar el modo dinámico
app.get('/modo_show_2', async (req, res) => {
  try {
    // Hacer una petición al ESP32 para activar el modo dinámico
    await axios.get(`${ESP32_IP}/modo_show_2`);
    console.log("Modo show 2 activado")
    res.send('Modo show 2 activado');
  } catch (error) {
    console.log(error.message);
    res.status(500).send('Error al activar el modo show');
  }
});

// Servir archivos estáticos desde la carpeta "public"
app.use('/media', express.static('media'));

// Iniciar el servidor en el puerto 3000
const port = 3000;
app.listen(port, () => {
  console.log(`Servidor backend escuchando en http://localhost:${port}`);
});
