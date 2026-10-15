# TP Integrador — Buffet Albert Einstein

> **Grupo:** B
> **Integrantes:**
> - Aylen Cortez — [aylulucortez](https://github.com/aylulucortez)

La consigna completa está en `enunciado.pdf` y los datos provistos por la cátedra en `datos/`.

## Quién hizo cada parte

| Programa | Integrante |
|---|---|
| normalizacion.cpp | Aylen Cortez |
| ventas.cpp | Aylen Cortez |
| cierre.cpp | Aylen Cortez |
| resumen.cpp | Aylen Cortez |

## Cómo compilar y correr

Copiar los datos de la cátedra a la raíz y compilar cada programa (cada uno tiene su `main`):

```bash
# PowerShell (Windows)
Copy-Item datos\*.dat .
# Git Bash / Linux
cp datos/comandas_historicas.dat datos/inventario.dat .

g++ -o normalizacion normalizacion.cpp
g++ -o ventas        ventas.cpp
g++ -o cierre        cierre.cpp
g++ -o resumen       resumen.cpp
```

Orden: `normalizacion` → `ventas` (las veces que haga falta) → `cierre` → `resumen`.

`normalizacion` se corre **una sola vez** sobre los datos originales (si se corre dos veces descuenta el stock dos veces).

## Cómo lo resolvimos

**normalizacion.cpp**
- Carga `comandas_historicas.dat` en un arreglo (`while (fread(...) == 1)`).
- Mozos: recorre las comandas y busca cada nombre en la lista (búsqueda secuencial, porque la lista no está ordenada por nombre). Si no está, lo agrega con `idMozo` = orden de aparición (1, 2, 3...) y si está le suma la comisión. Como los ids son consecutivos, el mozo N queda en la posición N-1 de `mozos.dat` y después se lee con acceso directo (`fseek`).
- Planillas por día: junta las fechas distintas; por cada una arma un arreglo con las ventas de ese día (id en vez de nombre), lo ordena por mozo (burbujeo) y lo graba en `comandas_dd-mm-aaaa.dat`.
- Stock: `inventario.dat` está ordenado por código con huecos, así que por cada venta se hace **búsqueda binaria sobre el archivo** con `fseek`, se descuenta la cantidad y se reescribe solo ese registro (`r+b`), sin rehacer el archivo. Si el stock da negativo queda en 0 y avisa.

**ventas.cpp**
- Pide la fecha una vez (valida formato y que exista) y arma el nombre de la planilla. Si la planilla de ese día no existe, la crea vacía al empezar (`fopen(..., "wb")`); cada venta se agrega al final con `fopen(..., "ab")`.
- Menú: cargar venta, ver productos, ver planilla, terminar.
- Login: el mozo se lee por acceso directo; si el número está fuera de rango no existe. La clave tipeada se encripta y se compara con la guardada.
- Producto por búsqueda binaria en el archivo; rechaza si no existe o no tiene stock, y la cantidad no puede superar el stock. Comisión = precio × cantidad × 10%. Se descuenta el stock y se suma la comisión al mozo en `mozos.dat`.
- Al terminar, ordena la planilla por mozo con **inserción** (la planilla ya estaba ordenada y lo nuevo queda al final, así que casi no mueve nada).

**cierre.cpp**
- Pide la fecha del primer día y el número de semana; genera los 7 días seguidos (pasando de mes si hace falta). Los días sin planilla se saltean.
- Como cada planilla ya está ordenada por mozo, hace un **apareo (merge)** de todas: toma el registro de menor `idMozo` entre los actuales, lo graba y avanza en esa planilla. El semanal sale ordenado sin volver a ordenar.

**resumen.cpp**
- Recorre `comandas_semana_sX-mm.dat` con **corte de control** por `idMozo`, suma productos y comisión de cada mozo y el total del buffet. El último mozo se muestra cuando se termina el archivo (el `while` interno corta por `hayDatos`).

## Claves

Corrimiento con **K = 5**: a cada carácter se le suma 5. La clave inicial de cada mozo es su número como texto.

Ejemplo, mozo 1: `'1'` (49) + 5 = 54 → `'6'`. En `mozos.dat` se guarda `"6"`.
Login: el mozo escribe `1` → +5 → `"6"` → coincide con lo guardado → entra.

Se puede deshacer restando 5 a cada carácter, porque a cada carácter le corresponde uno solo distinto (no se pierde información, como pasaría dividiendo).

## Casos raros

| Caso | Qué pasa |
|---|---|
| Mozo que no existe | `ventas` avisa y vuelve al menú |
| Clave incorrecta | `ventas` avisa y no deja cargar |
| Producto sin stock / inexistente | `ventas` no deja cargar la venta |
| Planilla del día no creada | `ventas` la crea; `cierre` saltea ese día |
| Último mozo del cierre | `resumen` lo muestra al terminar el archivo |
