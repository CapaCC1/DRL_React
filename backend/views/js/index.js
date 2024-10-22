const grid = document.getElementById('grid');
const activarDinamicoBtn = document.getElementById('activarDinamico');

        // Crear las celdas de la matriz
        for (let y = 0; y < 8; y++) {
            for (let x = 0; x < 8; x++) {
                const cell = document.createElement('div');
                cell.classList.add('cell');
                
                // Agregar evento para cambiar el color al hacer clic en una celda
                cell.addEventListener('click', () => {
                    const r = Math.floor(Math.random() * 256);
                    const g = Math.floor(Math.random() * 256);
                    const b = Math.floor(Math.random() * 256);
                    cell.style.backgroundColor = `rgb(${r},${g},${b})`;
                    
                    // Enviar petición al backend para actualizar el LED
                    fetch(`/set_pixel?x=${x}&y=${y}&r=${r}&g=${g}&b=${b}`);
                });
                
                grid.appendChild(cell);
            }
        }


        activarDinamicoBtn.addEventListener('click', () => {
            fetch('/modo_dinamico')
                .then(response => response.text())
                .then(data => alert(data))
                .catch(err => console.error(err));
        });