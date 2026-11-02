#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

// Lee: comandas_historicas.dat, inventario.dat
// Genera: mozos.dat, comandas_dd-mm-aaaa.dat (uno por dia), inventario.dat (actualizado)

const int K = 5; // corrimiento para guardar las claves

struct ComandaHistorica
{
    char fecha[11]; // "DD-MM-AAAA"
    char nombreMozo[50];
    int codigoProducto;
    int cantidad;
    float comision;
};

struct Producto
{
    int codigo;
    char descripcion[50];
    float precio;
    int stockActual;
};

struct Mozo
{
    int idMozo;
    char nombre[50];
    char password[20];
    float totalComision;
};

struct Comanda
{
    int idMozo;
    int codigoProducto;
    int cantidad;
    float comision;
};

// ---------------- Claves ----------------

// A cada caracter le suma K. Se deshace restando K.
void encriptar(char clave[])
{
    int i = 0;
    while (clave[i] != '\0')
    {
        clave[i] = clave[i] + K;
        i++;
    }
}

// Pasa un numero a texto (ej: 12 -> "12")
void numeroATexto(int numero, char texto[])
{
    char aux[20];
    int largo = 0;
    if (numero == 0)
    {
        aux[largo] = '0';
        largo++;
    }
    while (numero > 0)
    {
        aux[largo] = '0' + numero % 10;
        numero = numero / 10;
        largo++;
    }
    // los digitos quedaron al reves
    for (int i = 0; i < largo; i++)
    {
        texto[i] = aux[largo - 1 - i];
    }
    texto[largo] = '\0';
}

// ---------------- Lectura del historico ----------------

// Cuenta cuantas comandas tiene el historico sin leerlas:
// tamanio del archivo / tamanio de una comanda. Devuelve -1 si no existe.
int contarHistoricas()
{
    FILE *archivo = fopen("comandas_historicas.dat", "rb");
    if (archivo == NULL)
    {
        cout << "No se pudo abrir comandas_historicas.dat" << endl;
        return -1;
    }
    fseek(archivo, 0, SEEK_END);
    int total = ftell(archivo) / sizeof(ComandaHistorica);
    fclose(archivo);
    return total;
}

// Carga todo el historico en el arreglo, que ya tiene lugar para "total" comandas
int cargarHistoricas(ComandaHistorica historicas[], int total)
{
    FILE *archivo = fopen("comandas_historicas.dat", "rb");
    if (archivo == NULL)
    {
        return 0;
    }

    int cantidad = 0;
    while (cantidad < total && fread(&historicas[cantidad], sizeof(ComandaHistorica), 1, archivo) == 1)
    {
        cantidad++;
    }
    fclose(archivo);
    return cantidad;
}

// ---------------- Mozos ----------------

// Busqueda secuencial por nombre (la lista de mozos no esta ordenada por nombre)
int buscarMozoPorNombre(Mozo mozos[], int cantidadMozos, char nombre[])
{
    for (int i = 0; i < cantidadMozos; i++)
    {
        if (strcmp(mozos[i].nombre, nombre) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Arma la lista de mozos sin repetir. El id es el orden de aparicion (1, 2, 3...),
// asi el mozo con id N queda en la posicion N-1 del archivo (acceso directo).
int armarMozos(ComandaHistorica historicas[], int cantidadHistoricas, Mozo mozos[])
{
    int cantidadMozos = 0;
    for (int i = 0; i < cantidadHistoricas; i++)
    {
        int pos = buscarMozoPorNombre(mozos, cantidadMozos, historicas[i].nombreMozo);
        if (pos == -1)
        {
            Mozo nuevo;
            nuevo.idMozo = cantidadMozos + 1;
            strcpy(nuevo.nombre, historicas[i].nombreMozo);
            // clave inicial: el numero de mozo como texto, ya encriptada
            numeroATexto(nuevo.idMozo, nuevo.password);
            encriptar(nuevo.password);
            nuevo.totalComision = historicas[i].comision;
            mozos[cantidadMozos] = nuevo;
            cantidadMozos++;
        }
        else
        {
            mozos[pos].totalComision += historicas[i].comision;
        }
    }
    return cantidadMozos;
}

void guardarMozos(Mozo mozos[], int cantidadMozos)
{
    FILE *archivo = fopen("mozos.dat", "wb");
    if (archivo == NULL)
    {
        cout << "No se pudo crear mozos.dat" << endl;
        return;
    }
    fwrite(mozos, sizeof(Mozo), cantidadMozos, archivo);
    fclose(archivo);

    cout << "\nmozos.dat creado:" << endl;
    for (int i = 0; i < cantidadMozos; i++)
    {
        cout << "  " << mozos[i].idMozo << " - " << mozos[i].nombre
             << " - comision acumulada: $" << mozos[i].totalComision << endl;
    }
}

// ---------------- Planillas por dia ----------------

bool fechaYaEsta(char fechas[][11], int cantidadFechas, char fecha[])
{
    for (int i = 0; i < cantidadFechas; i++)
    {
        if (strcmp(fechas[i], fecha) == 0)
        {
            return true;
        }
    }
    return false;
}

// Burbujeo por idMozo
void ordenarPorMozo(Comanda comandas[], int cantidad)
{
    for (int i = 0; i < cantidad - 1; i++)
    {
        for (int j = 0; j < cantidad - i - 1; j++)
        {
            if (comandas[j].idMozo > comandas[j + 1].idMozo)
            {
                Comanda temp = comandas[j];
                comandas[j] = comandas[j + 1];
                comandas[j + 1] = temp;
            }
        }
    }
}

void armarPlanillasPorDia(ComandaHistorica historicas[], int cantidadHistoricas, Mozo mozos[], int cantidadMozos)
{
    // primero junto las fechas distintas.
    // Como mucho hay una fecha distinta por comanda: pido ese lugar.
    char (*fechas)[11] = new char[cantidadHistoricas][11];
    int cantidadFechas = 0;
    for (int i = 0; i < cantidadHistoricas; i++)
    {
        if (!fechaYaEsta(fechas, cantidadFechas, historicas[i].fecha))
        {
            strcpy(fechas[cantidadFechas], historicas[i].fecha);
            cantidadFechas++;
        }
    }

    // Un dia tiene como mucho todas las comandas: reuso este arreglo para cada dia
    Comanda *delDia = new Comanda[cantidadHistoricas];

    cout << "\nPlanillas por dia:" << endl;
    // por cada fecha: junto sus ventas, cambio nombre por id, ordeno y grabo
    for (int f = 0; f < cantidadFechas; f++)
    {
        int cantidadDelDia = 0;
        for (int i = 0; i < cantidadHistoricas; i++)
        {
            if (strcmp(historicas[i].fecha, fechas[f]) == 0)
            {
                int pos = buscarMozoPorNombre(mozos, cantidadMozos, historicas[i].nombreMozo);
                delDia[cantidadDelDia].idMozo = mozos[pos].idMozo;
                delDia[cantidadDelDia].codigoProducto = historicas[i].codigoProducto;
                delDia[cantidadDelDia].cantidad = historicas[i].cantidad;
                delDia[cantidadDelDia].comision = historicas[i].comision;
                cantidadDelDia++;
            }
        }

        ordenarPorMozo(delDia, cantidadDelDia);

        char nombreArchivo[40] = "comandas_";
        strcat(nombreArchivo, fechas[f]);
        strcat(nombreArchivo, ".dat");

        FILE *archivo = fopen(nombreArchivo, "wb");
        if (archivo == NULL)
        {
            cout << "No se pudo crear " << nombreArchivo << endl;
        }
        else
        {
            fwrite(delDia, sizeof(Comanda), cantidadDelDia, archivo);
            fclose(archivo);
            cout << "  " << nombreArchivo << " (" << cantidadDelDia << " ventas)" << endl;
        }
    }

    delete[] delDia;
    delete[] fechas;
}

// ---------------- Stock ----------------

// Busqueda binaria directamente sobre el archivo (esta ordenado por codigo, con huecos).
// Devuelve la posicion del registro o -1 si no existe.
int buscarProducto(FILE *archivo, int codigo, Producto &producto)
{
    fseek(archivo, 0, SEEK_END);
    int cantidad = ftell(archivo) / sizeof(Producto);

    int desde = 0;
    int hasta = cantidad - 1;
    while (desde <= hasta)
    {
        int medio = (desde + hasta) / 2;
        fseek(archivo, medio * sizeof(Producto), SEEK_SET);
        fread(&producto, sizeof(Producto), 1, archivo);
        if (producto.codigo == codigo)
        {
            return medio;
        }
        else if (producto.codigo < codigo)
        {
            desde = medio + 1;
        }
        else
        {
            hasta = medio - 1;
        }
    }
    return -1;
}

// Descuenta del inventario lo vendido, modificando solo el registro de cada producto.
void actualizarStock(ComandaHistorica historicas[], int cantidadHistoricas)
{
    FILE *archivo = fopen("inventario.dat", "r+b");
    if (archivo == NULL)
    {
        cout << "No se pudo abrir inventario.dat" << endl;
        return;
    }

    for (int i = 0; i < cantidadHistoricas; i++)
    {
        Producto producto;
        int pos = buscarProducto(archivo, historicas[i].codigoProducto, producto);
        if (pos == -1)
        {
            cout << "  Producto " << historicas[i].codigoProducto << " no existe en el inventario" << endl;
        }
        else
        {
            producto.stockActual -= historicas[i].cantidad;
            if (producto.stockActual < 0)
            {
                cout << "  El producto " << producto.descripcion << " se quedo sin stock" << endl;
                producto.stockActual = 0;
            }
            fseek(archivo, pos * sizeof(Producto), SEEK_SET);
            fwrite(&producto, sizeof(Producto), 1, archivo);
        }
    }

    // muestro como quedo
    cout << "\nInventario actualizado:" << endl;
    fseek(archivo, 0, SEEK_SET);
    Producto producto;
    while (fread(&producto, sizeof(Producto), 1, archivo) == 1)
    {
        cout << "  " << producto.codigo << " " << producto.descripcion
             << " $" << producto.precio << " stock: " << producto.stockActual << endl;
    }
    fclose(archivo);
}

int main()
{
    // Primero cuento las comandas y despues pido memoria justa para todas:
    // asi no hay un maximo fijo.
    int total = contarHistoricas();
    if (total == -1)
    {
        return 1;
    }
    ComandaHistorica *historicas = new ComandaHistorica[total];
    int cantidadHistoricas = cargarHistoricas(historicas, total);
    cout << "Se leyeron " << cantidadHistoricas << " comandas historicas." << endl;

    // Como mucho hay un mozo distinto por comanda
    Mozo *mozos = new Mozo[cantidadHistoricas];
    int cantidadMozos = armarMozos(historicas, cantidadHistoricas, mozos);
    guardarMozos(mozos, cantidadMozos);

    armarPlanillasPorDia(historicas, cantidadHistoricas, mozos, cantidadMozos);

    actualizarStock(historicas, cantidadHistoricas);

    delete[] mozos;
    delete[] historicas;
    return 0;
}
