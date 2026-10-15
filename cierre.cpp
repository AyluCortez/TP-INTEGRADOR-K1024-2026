#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

// Lee: comandas_dd-mm-aaaa.dat (los 7 dias de la semana)
// Genera: comandas_semana_sX-mm.dat

const int DIAS_SEMANA = 7;

struct Comanda
{
    int idMozo;
    int codigoProducto;
    int cantidad;
    float comision;
};

bool esBisiesto(int anio)
{
    return (anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0);
}

int diasDelMes(int mes, int anio)
{
    int diasEnMes[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (mes == 2 && esBisiesto(anio))
    {
        return 29;
    }
    return diasEnMes[mes - 1];
}

bool verificarFecha(int dia, int mes, int anio)
{
    return mes >= 1 && mes <= 12 && dia >= 1 && dia <= diasDelMes(mes, anio);
}

// Escribe un numero de 2 digitos en texto[0] y texto[1]
void dosDigitos(int numero, char texto[])
{
    texto[0] = '0' + numero / 10;
    texto[1] = '0' + numero % 10;
}

// Arma "comandas_dd-mm-aaaa.dat"
void armarNombreDia(int dia, int mes, int anio, char nombre[])
{
    char fecha[11];
    dosDigitos(dia, fecha);
    fecha[2] = '-';
    dosDigitos(mes, fecha + 3);
    fecha[5] = '-';
    dosDigitos(anio / 100, fecha + 6);
    dosDigitos(anio % 100, fecha + 8);
    fecha[10] = '\0';

    strcpy(nombre, "comandas_");
    strcat(nombre, fecha);
    strcat(nombre, ".dat");
}

// Arma "comandas_semana_sX-mm.dat"
void armarNombreSemana(int semana, int mes, char nombre[])
{
    char mm[3];
    dosDigitos(mes, mm);
    mm[2] = '\0';

    char sem[3];
    if (semana < 10)
    {
        sem[0] = '0' + semana;
        sem[1] = '\0';
    }
    else
    {
        dosDigitos(semana, sem);
        sem[2] = '\0';
    }

    strcpy(nombre, "comandas_semana_s");
    strcat(nombre, sem);
    strcat(nombre, "-");
    strcat(nombre, mm);
    strcat(nombre, ".dat");
}

void siguienteDia(int &dia, int &mes, int &anio)
{
    dia++;
    if (dia > diasDelMes(mes, anio))
    {
        dia = 1;
        mes++;
        if (mes > 12)
        {
            mes = 1;
            anio++;
        }
    }
}

int main()
{
    int dia, mes, anio;
    char guion;
    cout << "Fecha del primer dia de la semana (DD-MM-AAAA): ";
    cin >> dia >> guion >> mes >> guion >> anio;
    while (!verificarFecha(dia, mes, anio))
    {
        cout << "Fecha invalida. Fecha del primer dia de la semana (DD-MM-AAAA): ";
        cin >> dia >> guion >> mes >> guion >> anio;
    }

    int semana;
    cout << "Numero de semana del mes (1 a 5): ";
    cin >> semana;
    while (semana < 1 || semana > 5)
    {
        cout << "Numero invalido (1 a 5): ";
        cin >> semana;
    }

    char nombreSemana[40];
    armarNombreSemana(semana, mes, nombreSemana);

    // Abro las planillas de los 7 dias. Si un dia no tiene planilla, lo salteo.
    FILE *planillas[DIAS_SEMANA];
    int cantidadPlanillas = 0;
    for (int i = 0; i < DIAS_SEMANA; i++)
    {
        char nombreDia[40];
        armarNombreDia(dia, mes, anio, nombreDia);
        FILE *archivo = fopen(nombreDia, "rb");
        if (archivo == NULL)
        {
            cout << "  " << nombreDia << ": no existe, se saltea" << endl;
        }
        else
        {
            cout << "  " << nombreDia << ": ok" << endl;
            planillas[cantidadPlanillas] = archivo;
            cantidadPlanillas++;
        }
        siguienteDia(dia, mes, anio);
    }

    if (cantidadPlanillas == 0)
    {
        cout << "No hay ninguna planilla de esa semana." << endl;
        return 1;
    }

    FILE *salida = fopen(nombreSemana, "wb");
    if (salida == NULL)
    {
        cout << "No se pudo crear " << nombreSemana << endl;
        return 1;
    }

    // Apareo (merge) de varias planillas: como cada una ya esta ordenada por mozo,
    // miro el registro actual de cada una, grabo el de menor idMozo y avanzo solo en esa.
    // Asi el archivo semanal sale ordenado sin tener que ordenar de nuevo.
    Comanda actual[DIAS_SEMANA];
    bool hayDatos[DIAS_SEMANA];
    for (int i = 0; i < cantidadPlanillas; i++)
    {
        hayDatos[i] = fread(&actual[i], sizeof(Comanda), 1, planillas[i]) == 1;
    }

    int total = 0;
    bool quedan = true;
    while (quedan)
    {
        int menor = -1;
        for (int i = 0; i < cantidadPlanillas; i++)
        {
            if (hayDatos[i] && (menor == -1 || actual[i].idMozo < actual[menor].idMozo))
            {
                menor = i;
            }
        }

        if (menor == -1)
        {
            quedan = false; // se terminaron todas las planillas
        }
        else
        {
            fwrite(&actual[menor], sizeof(Comanda), 1, salida);
            total++;
            hayDatos[menor] = fread(&actual[menor], sizeof(Comanda), 1, planillas[menor]) == 1;
        }
    }

    for (int i = 0; i < cantidadPlanillas; i++)
    {
        fclose(planillas[i]);
    }
    fclose(salida);

    cout << "\nSe creo " << nombreSemana << " con " << total << " ventas ordenadas por mozo." << endl;
    return 0;
}
