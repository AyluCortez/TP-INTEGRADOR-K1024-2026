#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

// Lee: mozos.dat, inventario.dat
// Genera: comandas_dd-mm-aaaa.dat (la crea o le agrega), inventario.dat (actualizado)

const float TASA_COMISION = 0.10f; // la comision de cada venta es el 10% de lo vendido
const int K = 5;                   // mismo corrimiento que en normalizacion.cpp
const int MAX_COMANDAS = 2000;

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

// ---------------- Fecha ----------------

bool esBisiesto(int anio)
{
    return (anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0);
}

bool esDigito(char c)
{
    return c >= '0' && c <= '9';
}

// Verifica el formato DD-MM-AAAA y que la fecha exista
bool verificarFecha(char fecha[])
{
    if (strlen(fecha) != 10 || fecha[2] != '-' || fecha[5] != '-')
    {
        return false;
    }
    for (int i = 0; i < 10; i++)
    {
        if (i != 2 && i != 5 && !esDigito(fecha[i]))
        {
            return false;
        }
    }
    int dia = (fecha[0] - '0') * 10 + (fecha[1] - '0');
    int mes = (fecha[3] - '0') * 10 + (fecha[4] - '0');
    int anio = (fecha[6] - '0') * 1000 + (fecha[7] - '0') * 100 + (fecha[8] - '0') * 10 + (fecha[9] - '0');

    if (mes < 1 || mes > 12)
    {
        return false;
    }
    int diasEnMes[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (esBisiesto(anio))
    {
        diasEnMes[1] = 29;
    }
    return dia >= 1 && dia <= diasEnMes[mes - 1];
}

void pedirFecha(char fecha[])
{
    cout << "Fecha del dia (DD-MM-AAAA): ";
    cin >> fecha;
    while (!verificarFecha(fecha))
    {
        cout << "Fecha invalida. Fecha del dia (DD-MM-AAAA): ";
        cin >> fecha;
    }
}

// ---------------- Claves ----------------

void encriptar(char clave[])
{
    int i = 0;
    while (clave[i] != '\0')
    {
        clave[i] = clave[i] + K;
        i++;
    }
}

// ---------------- Mozos ----------------

// Los ids van de 1 a N en orden, asi que el mozo con id N esta en la posicion N-1:
// se lee directo con fseek, sin recorrer el archivo.
bool leerMozo(int idMozo, Mozo &mozo)
{
    FILE *archivo = fopen("mozos.dat", "rb");
    if (archivo == NULL)
    {
        cout << "No se pudo abrir mozos.dat (corre primero normalizacion)" << endl;
        return false;
    }
    fseek(archivo, 0, SEEK_END);
    int cantidad = ftell(archivo) / sizeof(Mozo);
    if (idMozo < 1 || idMozo > cantidad)
    {
        fclose(archivo);
        return false;
    }
    fseek(archivo, (idMozo - 1) * sizeof(Mozo), SEEK_SET);
    fread(&mozo, sizeof(Mozo), 1, archivo);
    fclose(archivo);
    return true;
}

void sumarComisionAlMozo(int idMozo, float comision)
{
    FILE *archivo = fopen("mozos.dat", "r+b");
    if (archivo == NULL)
    {
        return;
    }
    Mozo mozo;
    fseek(archivo, (idMozo - 1) * sizeof(Mozo), SEEK_SET);
    fread(&mozo, sizeof(Mozo), 1, archivo);
    mozo.totalComision += comision;
    fseek(archivo, (idMozo - 1) * sizeof(Mozo), SEEK_SET);
    fwrite(&mozo, sizeof(Mozo), 1, archivo);
    fclose(archivo);
}

// Pide numero de mozo y clave. Devuelve el id o -1 si no pudo entrar.
int login()
{
    int idMozo;
    Mozo mozo;
    cout << "\nNumero de mozo: ";
    cin >> idMozo;
    if (!leerMozo(idMozo, mozo))
    {
        cout << "El mozo " << idMozo << " no existe." << endl;
        return -1;
    }

    char clave[20];
    cout << "Clave: ";
    cin >> clave;
    encriptar(clave); // le hago la misma cuenta y comparo con lo guardado
    if (strcmp(clave, mozo.password) != 0)
    {
        cout << "Clave incorrecta." << endl;
        return -1;
    }
    cout << "Hola " << mozo.nombre << "!" << endl;
    return idMozo;
}

// ---------------- Productos ----------------

// Busqueda binaria sobre el archivo (ordenado por codigo, con huecos)
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

void mostrarInventario()
{
    FILE *archivo = fopen("inventario.dat", "rb");
    if (archivo == NULL)
    {
        cout << "No se pudo abrir inventario.dat" << endl;
        return;
    }
    Producto producto;
    cout << "\nCodigo  Producto          Precio   Stock" << endl;
    while (fread(&producto, sizeof(Producto), 1, archivo) == 1)
    {
        cout << producto.codigo << "     " << producto.descripcion
             << "  $" << producto.precio << "  " << producto.stockActual << endl;
    }
    fclose(archivo);
}

// ---------------- Carga de una venta ----------------

void cargarVenta(char nombrePlanilla[])
{
    int idMozo = login();
    if (idMozo == -1)
    {
        return;
    }

    FILE *inventario = fopen("inventario.dat", "r+b");
    if (inventario == NULL)
    {
        cout << "No se pudo abrir inventario.dat" << endl;
        return;
    }

    int codigo;
    Producto producto;
    cout << "Codigo de producto: ";
    cin >> codigo;
    int pos = buscarProducto(inventario, codigo, producto);
    if (pos == -1)
    {
        cout << "El producto " << codigo << " no existe." << endl;
        fclose(inventario);
        return;
    }
    if (producto.stockActual == 0)
    {
        cout << producto.descripcion << " esta sin stock." << endl;
        fclose(inventario);
        return;
    }

    int cantidad;
    cout << producto.descripcion << " (stock " << producto.stockActual << ") - Cantidad: ";
    cin >> cantidad;
    while (cantidad <= 0 || cantidad > producto.stockActual)
    {
        cout << "Cantidad invalida (entre 1 y " << producto.stockActual << "): ";
        cin >> cantidad;
    }

    Comanda comanda;
    comanda.idMozo = idMozo;
    comanda.codigoProducto = codigo;
    comanda.cantidad = cantidad;
    comanda.comision = producto.precio * cantidad * TASA_COMISION;

    // "ab" agrega al final y, si la planilla no existe, la crea
    FILE *planilla = fopen(nombrePlanilla, "ab");
    if (planilla == NULL)
    {
        cout << "No se pudo abrir " << nombrePlanilla << endl;
        fclose(inventario);
        return;
    }
    fwrite(&comanda, sizeof(Comanda), 1, planilla);
    fclose(planilla);

    // descuento el stock solo en el registro de ese producto
    producto.stockActual -= cantidad;
    fseek(inventario, pos * sizeof(Producto), SEEK_SET);
    fwrite(&producto, sizeof(Producto), 1, inventario);
    fclose(inventario);

    sumarComisionAlMozo(idMozo, comanda.comision);

    cout << "Venta guardada. Comision: $" << comanda.comision << endl;
}

// ---------------- Planilla del dia ----------------

void mostrarPlanilla(char nombrePlanilla[])
{
    FILE *archivo = fopen(nombrePlanilla, "rb");
    if (archivo == NULL)
    {
        cout << "La planilla todavia no tiene ventas." << endl;
        return;
    }
    Comanda comanda;
    cout << "\nMozo  Producto  Cantidad  Comision" << endl;
    while (fread(&comanda, sizeof(Comanda), 1, archivo) == 1)
    {
        cout << comanda.idMozo << "     " << comanda.codigoProducto << "       "
             << comanda.cantidad << "         $" << comanda.comision << endl;
    }
    fclose(archivo);
}

// La planilla ya venia ordenada y las ventas nuevas quedaron al final:
// uso insercion, que en un arreglo casi ordenado hace pocos movimientos.
// Ademas no cambia el orden entre ventas del mismo mozo.
void ordenarPlanilla(char nombrePlanilla[])
{
    FILE *archivo = fopen(nombrePlanilla, "rb");
    if (archivo == NULL)
    {
        return;
    }
    Comanda comandas[MAX_COMANDAS];
    int cantidad = 0;
    Comanda aux;
    while (fread(&aux, sizeof(Comanda), 1, archivo) == 1 && cantidad < MAX_COMANDAS)
    {
        comandas[cantidad] = aux;
        cantidad++;
    }
    fclose(archivo);

    for (int i = 1; i < cantidad; i++)
    {
        Comanda actual = comandas[i];
        int j = i - 1;
        while (j >= 0 && comandas[j].idMozo > actual.idMozo)
        {
            comandas[j + 1] = comandas[j];
            j--;
        }
        comandas[j + 1] = actual;
    }

    archivo = fopen(nombrePlanilla, "wb");
    fwrite(comandas, sizeof(Comanda), cantidad, archivo);
    fclose(archivo);
    cout << "Planilla " << nombrePlanilla << " ordenada por mozo (" << cantidad << " ventas)." << endl;
}

int main()
{
    char fecha[20];
    pedirFecha(fecha);

    char nombrePlanilla[40] = "comandas_";
    strcat(nombrePlanilla, fecha);
    strcat(nombrePlanilla, ".dat");

    FILE *existe = fopen(nombrePlanilla, "rb");
    if (existe == NULL)
    {
        // la planilla de ese dia todavia no existe: la creo vacia
        FILE *nueva = fopen(nombrePlanilla, "wb");
        if (nueva == NULL)
        {
            cout << "No se pudo crear " << nombrePlanilla << endl;
            return 1;
        }
        fclose(nueva);
        cout << "No habia planilla para ese dia, se creo " << nombrePlanilla << endl;
    }
    else
    {
        cout << "Se agregan las ventas a " << nombrePlanilla << endl;
        fclose(existe);
    }

    int opcion;
    do
    {
        cout << "\n--- Ventas del " << fecha << " ---" << endl;
        cout << "1. Cargar venta" << endl;
        cout << "2. Ver productos" << endl;
        cout << "3. Ver planilla del dia" << endl;
        cout << "0. Terminar" << endl;
        cout << "Opcion: ";
        cin >> opcion;

        if (opcion == 1)
        {
            cargarVenta(nombrePlanilla);
        }
        else if (opcion == 2)
        {
            mostrarInventario();
        }
        else if (opcion == 3)
        {
            mostrarPlanilla(nombrePlanilla);
        }
        else if (opcion != 0)
        {
            cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);

    ordenarPlanilla(nombrePlanilla);
    return 0;
}
