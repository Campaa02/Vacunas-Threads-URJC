#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <errno.h>

//constantes
#define TANDAS 10 // lo dice el enunciado
#define CENTROS 5 // lo dice el enunciado
#define FABRICAS 3 // lo dice el enunciado
#define MAXBITS 6 //4 bits para limitar el nº maximo introducido --> [0,10000)
#define MINTREACCION 1 // lo dice el enunciado
#define MINTDESPLAZAR 1 // lo dice el enunciado

//Mutex y Variable Condición
pthread_cond_t *centOperativo;
pthread_mutex_t mutex;


//ficheros
FILE *fIn; //fichero de entrada
FILE *fOut; //fichero de salida

//variables
int habitantes; //numero de habitantes
int vacunadosXTanda; //numeros de vacunados en cada tanda
int inicVacunas; //numero inicial de vacunas en cada fabrica
int totVacunasFabrica; //numero total de vacunas fabricadas en cada fabrica 
int minVacunasXTanda; //numero minimo de habitantes vacunados por tanda
int maxVacunasXTanda; //numero maximo de habitantes vacunados por tanda
int minFabricarVacunas; //tiempo minimo requerido para fabricar vacunas
int maxFabricarVacunas; //tiempo maximo requerido para fabricar vacunas
int maxTReparto; //tiempo maximo requerido para repartir las vacunas
int maxTReaccion; //tiempo maximo requerido para que un habitante se de cuenta de que debe vacunarse
int maxDesplazar; //tiempo maximo requerido para que el usuario vaya a vacunarse
int *VacunasCentros; //registro del numero de vacunas de cada centro
int *listaHabitantes; //registro de los habitantes vacunados
int *vacunasFabricadas; //registro del numero de vacunas fabricadas por cada fabrica
int *listaFabricas; //registro de fabricas
int *listaVacunasRepartidasCentros; //registro del numero de vacunas repartidas a cada centro
int **vacunasEntregadas; //registro de vacunas entregadas por cada fabrica a cada centro


// Funciones auxiliares
void *vacunacion (void* args);
void *fabricacion (void* args);
int  cogerValores (FILE *fd);
void asignarValores (FILE *fd);
void mostrarInfo();
void imprimirEstadisticas ();



void *vacunacion (void* args) {

	int habitantes = *((int *) args);
	int i = (rand() % CENTROS); // Escoge un centro de vacunación aleatoriamente

	sleep(rand() % maxTReaccion + MINTREACCION); // Tiempo de reacción ante la cita para vacunarse
	printf("- Habitante %i elige el centro %i para vacunarse.\n", habitantes+1, i+1);
	fprintf(fOut,"- Habitante %i elige el centro %i para vacunarse.\n", habitantes+1, i+1);
	
	sleep(rand() % maxDesplazar + MINTDESPLAZAR); // Tiempo de desplazamiento

	pthread_mutex_lock(&mutex); // INICIO en la seccion critica (por el uso de variables globales)
	while (VacunasCentros[i] <= 0) {
		pthread_cond_wait(&centOperativo[i], &mutex);
	}
	// Si hay alguna vacuna disponible
	VacunasCentros[i]--; //Actualizamos, ya que una persona va a usarla
	pthread_cond_broadcast(&centOperativo[i]);
	
	pthread_mutex_unlock(&mutex); // FIN de la seccion critica
	
	
	printf("- Habitante %i vacunado en el centro %i.\n", habitantes+1, i+1);
	fprintf(fOut,"- Habitante %i vacunado en el centro %i.\n", habitantes+1, i+1);

	pthread_exit(NULL);

}

void *fabricacion (void* args) {

	int nFabrica = *((int *) args);
	int i,j;
	int *vacunas;
	 
	while (vacunasFabricadas[nFabrica] > 0) { // Si aún tiene que fabricar vacunas (la fábrica)
		//el num de vacunas que se generan x tanda
		int vacunasXTanda = rand() % (maxVacunasXTanda - minVacunasXTanda + 1) + minVacunasXTanda;
		int exc = 0; //excedentes
		
		vacunas = (int*) malloc (CENTROS * sizeof(int));

		sleep(rand() % (maxFabricarVacunas - minFabricarVacunas + 1) + minFabricarVacunas); // Tiempo en preparar las vacunas

		vacunasFabricadas[nFabrica] = vacunasFabricadas[nFabrica] -vacunasXTanda;
	
		if (vacunasFabricadas[nFabrica] < 0) { // Si nos pasamos de las vacunas asignadas a cada fábrica
			vacunasXTanda= vacunasXTanda + vacunasFabricadas[nFabrica];
			vacunasFabricadas[nFabrica] = 0;
		}

		exc = vacunasXTanda % CENTROS; // Las que sobran 

		pthread_mutex_lock(&mutex); // INICIO en la seccion critica

		if (vacunasXTanda > 0) {
			
			printf("+ Fábrica %i prepara %i vacunas.\n", nFabrica+1, vacunasXTanda);
			fprintf(fOut, "+ Fábrica %i prepara %i vacunas.\n", nFabrica+1, vacunasXTanda);

			sleep(rand() % maxTReparto + 1); // Tiempo de reparto de las vacunas

			for (i = 0; i <= CENTROS - 1; i++) {
					VacunasCentros[i] = VacunasCentros[i] + vacunasXTanda/CENTROS;
	      			listaVacunasRepartidasCentros[i] =listaVacunasRepartidasCentros[i]+ vacunasXTanda/CENTROS;
	      			vacunasEntregadas[nFabrica][i] = vacunasEntregadas[nFabrica][i]+vacunasXTanda/CENTROS;
	      			vacunas[i] = vacunasXTanda/CENTROS;
	    		}

			while (exc > 0) {
		    // suponemos que el centro con menos vacunas es el primero
			int min = VacunasCentros[0];     
			int aux = 0;
			//y ahora  comparamos el supuesto min con TODOS los centros y si alguno es menor
			//guardamos su posición en aux.
			for (int i = 0; i <= CENTROS - 1; i++) {         
				if (VacunasCentros[i] < min) {
				min = VacunasCentros[i];
				aux = i;
				}
			}
			//Para saber a que centro ir
			j =aux;
					
	  			VacunasCentros[j]++;
		  		listaVacunasRepartidasCentros[j]++;
		  		vacunasEntregadas[nFabrica][j]++;
		  		vacunas[j]++;
	  		//reducimos el num de excedentes
	  			exc--;
	  		}
	  		
	  		for (i = 0; i <= CENTROS - 1; i++) {
		  		printf("+ Fábrica %i entrega %i vacunas en el centro %i.\n", nFabrica+1, vacunas[i], i+1);
		  		fprintf(fOut,"+ Fábrica %i entrega %i vacunas en el centro %i.\n", nFabrica+1, vacunas[i], i+1);
			} 
		}

		if (vacunasFabricadas[nFabrica] == 0) { // Si hemos llegado al límite de vacunas preparadas por cada fábrica
			printf("+ Fábrica %i ha fabricado todas sus vacunas.\n", nFabrica+1);
			fprintf(fOut,"+ Fábrica %i ha fabricado todas sus vacunas.\n", nFabrica+1);
		}

		pthread_mutex_unlock(&mutex); // FIN de la seccion critica
	    //Mandamos la señal una vez hemos terminado de repartir las vacunas pertinentes
		for (i = 0; i <= CENTROS - 1; i++) { 
			pthread_cond_broadcast(&centOperativo[i]);
		}
		//liberamos la memoria dinámica reservada previamente con el malloc
		free(vacunas);
	}

	pthread_exit(NULL);

}


void mostrarInfo () {

	vacunadosXTanda = habitantes/TANDAS;
	totVacunasFabrica = habitantes/FABRICAS;

	printf("*****VACUNACION EN PANDEMIA: CONFIGURACION INICIAL*****\n");
    	fprintf(fOut,"*****VACUNACION EN PANDEMIA: CONFIGURACION INICIAL*****\n");
        printf("Habitantes: %i \n", habitantes);
        fprintf(fOut,"Habitantes: %i \n", habitantes);
        printf("Centros de Vacunacion: %i \n", CENTROS);
        fprintf(fOut,"Centros de Vacunacion: %i \n", CENTROS);
        printf("Fabricas: %i \n", FABRICAS);
        fprintf(fOut,"Fabricas: %i \n", FABRICAS);
        printf("Vacunados por tanda: %i \n", vacunadosXTanda);
        fprintf(fOut,"Vacunados por tanda: %i \n", vacunadosXTanda);
        printf("Vacunas Iniciales en cada centro: %i \n", inicVacunas);
        fprintf(fOut,"Vacunas Iniciales en cada centro: %i \n", inicVacunas);
        printf("Vacunas Totales por fabrica: %i \n", totVacunasFabrica);
        fprintf(fOut,"Vacunas Totales por fabrica: %i \n", totVacunasFabrica);
		printf("Mínimo número de vacunas fabricadas en cada tanda: %i \n",minVacunasXTanda);
        fprintf(fOut, "Mínimo número de vacunas fabricadas en cada tanda: %i \n",minVacunasXTanda);
        printf("Máximo número de vacunas fabricadas en cada tanda: %i \n",maxVacunasXTanda);
        fprintf(fOut,"Máximo número de vacunas fabricadas en cada tanda: %i \n",maxVacunasXTanda);
        printf("Tiempo mínimo de fabricación de una tanda de vacunas: %i \n", minFabricarVacunas);
        fprintf(fOut,"Tiempo mínimo de fabricación de una tanda de vacunas: %i \n", minFabricarVacunas);
        printf("Tiempo máximo de fabricación de una tanda de vacunas: %i \n", maxFabricarVacunas);
        fprintf(fOut,"Tiempo máximo de fabricación de una tanda de vacunas: %i \n", maxFabricarVacunas);
        printf("Tiempo máximo de reparto de vacunas a los centros: %i \n", maxTReparto);
        fprintf(fOut,"Tiempo máximo de reparto de vacunas a los centros: %i \n", maxTReparto);
        printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i \n", maxTReaccion);
        fprintf(fOut,"Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i \n", maxTReaccion);
        printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i \n",maxDesplazar);
        fprintf(fOut,"Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i \n",maxDesplazar);
        printf("\n");
        fprintf(fOut,"\n");
        printf("*****PROCESO DE VACUNACION*****\n");
        fprintf(fOut,"*****PROCESO DE VACUNACION*****\n");
}
int cogerValores (FILE *fd) {

	int aux;
	char buffer[100];

	fgets(buffer,MAXBITS,fd); 
	//transforma String a Int
	aux = atoi(buffer);

	return aux;	
}
void asignarValores (FILE *fd) {

	habitantes = cogerValores(fd);
	inicVacunas = cogerValores(fd);
	minVacunasXTanda = cogerValores(fd);
	maxVacunasXTanda = cogerValores(fd);
	minFabricarVacunas = cogerValores(fd);
	maxFabricarVacunas = cogerValores(fd);
	maxTReparto = cogerValores(fd);
	maxTReaccion = cogerValores(fd);
	maxDesplazar = cogerValores(fd);
}

void imprimirEstadisticas () {

	int i,j;
	
	totVacunasFabrica = habitantes/FABRICAS;
	printf("*****ESTADÍSTICAS DE VACUNACIÓN*****\n");
	fprintf(fOut, "*****ESTADÍSTICAS DE VACUNACIÓN*****\n");

	for (i = 0; i <= FABRICAS - 1; i++) {
		printf("\n+ Fábrica %i:\n", i+1);
		fprintf(fOut, "\n+ Fábrica %i:\n", i+1);
		printf("Ha fabricado %i vacunas.\n", totVacunasFabrica-vacunasFabricadas[i]);
		fprintf(fOut, "Ha fabricado %i vacunas.\n", totVacunasFabrica-vacunasFabricadas[i]);
		for (j = 0; j <= CENTROS - 1; j++) {
			printf("Fábrica %i ha entregado %i vacuna/s al centro %i\n", i+1,vacunasEntregadas[i][j],j+1);
			printf("Fábrica %i ha entregado %i vacuna/s al centro %i\n", i+1,vacunasEntregadas[i][j],j+1);
		}
	}

	for (i = 0; i <= CENTROS - 1; i++) {

		printf("\n- Centro %i:\n", i+1);
		fprintf(fOut, "\n- Centro %i:\n", i+1);
		printf("Ha recibido %i vacunas.\n", listaVacunasRepartidasCentros[i]);
		fprintf(fOut, "Ha recibido %i vacunas.\n", listaVacunasRepartidasCentros[i]);
		printf("Se han vacunado un total de %i habitantes.\n", (listaVacunasRepartidasCentros[i]+inicVacunas) - VacunasCentros[i]);
		fprintf(fOut, "Se han vacunado un total de %i habitantes.\n", (listaVacunasRepartidasCentros[i]+inicVacunas) - VacunasCentros[i]);
		printf("Han sobrado un total de %i vacunas.\n", VacunasCentros[i]);
		fprintf(fOut, "Han sobrado un total de %i vacunas.\n", VacunasCentros[i]);
	}
}

int main(int argc, char *argv[]) {

	// Variables locales
	pthread_t *th_listaFabricas;
	pthread_t *th_listaHabitantes;
	
	
	vacunadosXTanda = habitantes/TANDAS;
	totVacunasFabrica = habitantes/FABRICAS;
	
	int i,j;
	

	
	// Ficheros de entrada y de salida

	if (argc == 1) {
		fIn = fopen("entrada_vacunacion.txt", "a");
		fIn = fopen("entrada_vacunacion.txt", "r");
		fOut = fopen("salida_vacunacion.txt", "w");
	} else if (argc == 2) {
		fIn = fopen(argv[1], "r");
		fOut = fopen("salida_vacunacion.txt", "w");
	} else if (argc == 3) {
		fIn = fopen(argv[1], "r");
		fOut = fopen(argv[2], "w");
	} else {
		printf("Error en el número de argumentos.\n");
		exit(1);
	}
	if (fIn == NULL || fOut == NULL){
			fprintf(stderr,"Error en el fopen, %s no existe\n",argv[1]);
			exit(1);
			}

	// Guardamos los valores del fichero de entrada e imprimimos los datos por pantalla

	asignarValores(fIn);
	mostrarInfo();
	// Crea una semilla a partir de la hora en ña hora en la que ha sido creada
	srand(time(NULL)); 

	//Se asigna memoria

	th_listaFabricas = (pthread_t*) malloc (FABRICAS * sizeof(pthread_t));
	th_listaHabitantes = (pthread_t*) malloc (habitantes * sizeof(pthread_t));	
	vacunasFabricadas = (int*) malloc (FABRICAS * sizeof(int));
	VacunasCentros = (int*) malloc (CENTROS * sizeof(int));
	listaFabricas = (int*) malloc (FABRICAS * sizeof(int));
	listaHabitantes = (int*) malloc (habitantes * sizeof(int));
	centOperativo = (pthread_cond_t*) malloc (CENTROS * sizeof(pthread_cond_t));
	listaVacunasRepartidasCentros = (int*) malloc (CENTROS * sizeof(int));
	vacunasEntregadas = (int **) malloc (FABRICAS * sizeof(int*)); 
	for (i = 0; i <= CENTROS - 1; i++) {
		vacunasEntregadas[i] = (int*) malloc (CENTROS * sizeof(int)); 
	}

	// Se inicializa el mutex y las variables condición

	pthread_mutex_init(&mutex, NULL);

	for (i = 0; i <= CENTROS - 1; i++) {
		pthread_cond_init(&centOperativo[i], NULL);
	}

	// Las vacunas fabricadas y las vacunas entregadas a cada centro

	for (i = 0; i <= FABRICAS - 1; i++) {
		vacunasFabricadas[i] = totVacunasFabrica;
		for (j = 0; j <= CENTROS - 1; j++) {
			vacunasEntregadas[i][j] = 0;
		}
	}
	
	//Vacunas de los centros acorde a los datos del fichero 
	//y ponemos las vacunas de cada centro a 0

	for (i = 0; i <= CENTROS - 1; i++) {
		VacunasCentros[i] = inicVacunas;
		listaVacunasRepartidasCentros[i] = 0;
	}

	// Se crean las fabricas
	for (i = 0; i <= FABRICAS - 1; i++) {
		listaFabricas[i] = i;
		pthread_create(&th_listaFabricas[i], NULL, fabricacion, (void *) &listaFabricas[i]);
	}
	
	// Se crean los habitantes

	for (i = 0; i <= TANDAS - 1; i++) { 
		for (j = 0; j <= vacunadosXTanda - 1; j++) { 
			listaHabitantes[j] = j + (vacunadosXTanda*i);
			pthread_create(&th_listaHabitantes[j], NULL, vacunacion, (void *) &listaHabitantes[j]);
		}
		for (j = 0; j <= vacunadosXTanda - 1; j++) {
			//Duerme hasta que otro thread termine
			pthread_join(th_listaHabitantes[j],NULL);
		}	
	}

		for (i = 0; i <= FABRICAS - 1; i++) {
			//Duerme hasta que otro thread termine
			pthread_join(th_listaFabricas[i], NULL);
		}

	printf("Vacunacion finalizada.\n\n");
	fprintf(fOut, "Vacunacion finalizada.\n\n");

	// Mostramos las estadísticas pedidas
	imprimirEstadisticas();

	// Liberamos memoria dinámica
	
	free(th_listaHabitantes);
	free(th_listaFabricas);
	free(VacunasCentros);
	free(vacunasFabricadas);
	free(listaHabitantes);
	free(listaFabricas);
	free(centOperativo);
	free(vacunasEntregadas);

	// Destruimos las variables condición y el mutex
	
	for (i = 0; i <= CENTROS - 1; i++) {
		pthread_cond_destroy(&centOperativo[i]);
	}

	pthread_mutex_destroy(&mutex);
	
	//Finalmente cerramos los FILE*
	fclose(fIn);
	fclose(fOut);
	
	exit(0);
}

