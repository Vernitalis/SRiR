#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TAG_TASK 1
#define TAG_RESULT 2
#define TAG_DONE 3

typedef struct {
    int width, height, max_iter;
    double xmin, xmax, ymin, ymax;
} Config;

void read_config(const char* filename, Config* cfg) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Blad otwarcia pliku konfiguracyjnego!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fscanf(file, "%d\n%d\n%lf\n%lf\n%lf\n%lf\n%d", 
           &cfg->width, &cfg->height, &cfg->xmin, &cfg->xmax, 
           &cfg->ymin, &cfg->ymax, &cfg->max_iter);
    fclose(file);
}

void save_image(const char* filename, int* image, Config cfg) {
    FILE* file = fopen(filename, "wb");
    fprintf(file, "P6\n%d %d\n255\n", cfg.width, cfg.height);
    for (int i = 0; i < cfg.width * cfg.height; i++) {
        int iter = image[i];
        unsigned char r = (iter % 256);
        unsigned char g = ((iter * 2) % 256);
        unsigned char b = ((iter * 5) % 256);
        if (iter == cfg.max_iter) { r = g = b = 0; } // Wnętrze zbioru - czarne
        fwrite(&r, 1, 1, file);
        fwrite(&g, 1, 1, file);
        fwrite(&b, 1, 1, file);
    }
    fclose(file);
}

int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) printf("Wymagane sa przynajmniej 2 procesy (1 Manager, 1+ Workerow)!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    Config cfg;

    // INTERFEJS UŻYTKOWNIKA
    if (rank == 0) {
        printf("\n===================================================\n");
        printf("   GENERATOR FRAKTALI: ZBIOR MANDELBROTA (MPI+OMP)\n");
        printf("===================================================\n\n");

        if (argc > 1) {
            printf("[UI] Wczytywanie konfiguracji z pliku: %s\n", argv[1]);
            read_config(argv[1], &cfg);
        } else {
            int choice;
            printf("Brak pliku w argumentach wywolania.\n");
            printf("Wybierz tryb pracy:\n");
            printf("  1. Wczytaj domyslny plik (config.txt)\n");
            printf("  2. Wprowadz parametry recznie\n");
            printf("Twoj wybor: ");
            fflush(stdout);
            
            if (scanf("%d", &choice) != 1) choice = 1; // Zabezpieczenie

            if (choice == 2) {
                printf("\n--- Konfiguracja reczna ---\n");
                printf("Szerokosc obrazu (px): "); scanf("%d", &cfg.width);
                printf("Wysokosc obrazu (px): "); scanf("%d", &cfg.height);
                printf("Min X (np. -2.0): "); scanf("%lf", &cfg.xmin);
                printf("Max X (np. 1.0): "); scanf("%lf", &cfg.xmax);
                printf("Min Y (np. -1.5): "); scanf("%lf", &cfg.ymin);
                printf("Max Y (np. 1.5): "); scanf("%lf", &cfg.ymax);
                printf("Maksymalna liczba iteracji (np. 1000): "); scanf("%d", &cfg.max_iter);
            } else {
                printf("[UI] Wczytywanie z config.txt...\n");
                read_config("config.txt", &cfg);
            }
        }
        printf("\n[UI] Rozpoczynam obliczenia z uzyciem %d procesow...\n\n", size);
    }

    // Rozgłoszenie konfiguracji do wszystkich procesów
    MPI_Bcast(&cfg, sizeof(Config), MPI_BYTE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        // MANAGER 
        int* image = (int*)malloc(cfg.width * cfg.height * sizeof(int));
        int row_to_send = 0;
        int active_workers = size - 1;
        int rows_completed = 0;
        
        int* recv_buf = (int*)malloc((cfg.width + 1) * sizeof(int));
        MPI_Status status;

        // wysłanie zadań
        for (int i = 1; i < size; i++) {
            MPI_Send(&row_to_send, 1, MPI_INT, i, TAG_TASK, MPI_COMM_WORLD);
            row_to_send++;
        }

        // Pętla nasłuchująca i interfejs paska postępu
        while (active_workers > 0) {
            MPI_Recv(recv_buf, cfg.width + 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            if (status.MPI_TAG == TAG_RESULT) {
                int row_index = recv_buf[0];
                for (int x = 0; x < cfg.width; x++) {
                    image[row_index * cfg.width + x] = recv_buf[x + 1];
                }
                
                rows_completed++;

                // INTERFEJS UŻYTKOWNIKA (PASEK POSTĘPU) 
                int percent = (rows_completed * 100) / cfg.height;
                int bar_width = 50;
                int pos = (bar_width * rows_completed) / cfg.height;
                
                printf("\r[UI] Postep: [");
                for (int i = 0; i < bar_width; i++) {
                    if (i < pos) printf("=");
                    else if (i == pos) printf(">");
                    else printf(" ");
                }
                printf("] %3d%%", percent);
                fflush(stdout); // Wymuszenie odświeżenia linii w terminalu

                if (row_to_send < cfg.height) {
                    MPI_Send(&row_to_send, 1, MPI_INT, status.MPI_SOURCE, TAG_TASK, MPI_COMM_WORLD);
                    row_to_send++;
                } else {
                    int dummy = -1;
                    MPI_Send(&dummy, 1, MPI_INT, status.MPI_SOURCE, TAG_DONE, MPI_COMM_WORLD);
                    active_workers--;
                }
            }
        }

        printf("\n\n[UI] Obliczenia zakonczone. Zapisywanie obrazu...\n");
        save_image("mandelbrot.ppm", image, cfg);
        printf("[UI] Gotowe! Obraz zapisano jako mandelbrot.ppm\n");
        
        free(image);
        free(recv_buf);

    } else {
        // WORKER
        int row_index;
        MPI_Status status;
        int* send_buf = (int*)malloc((cfg.width + 1) * sizeof(int));

        while (1) {
            MPI_Recv(&row_index, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG_DONE) break;

            send_buf[0] = row_index;
            double dy = cfg.ymax - cfg.ymin;
            double dx = cfg.xmax - cfg.xmin;
            double cy = cfg.ymin + row_index * dy / (cfg.height - 1);

            #pragma omp parallel for schedule(dynamic)
            for (int x = 0; x < cfg.width; x++) {
                double cx = cfg.xmin + x * dx / (cfg.width - 1);
                double zx = 0.0, zy = 0.0;
                int iter = 0;

                while (zx * zx + zy * zy <= 4.0 && iter < cfg.max_iter) {
                    double tmp = zx * zx - zy * zy + cx;
                    zy = 2.0 * zx * zy + cy;
                    zx = tmp;
                    iter++;
                }
                send_buf[x + 1] = iter;
            }

            MPI_Send(send_buf, cfg.width + 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        }
        free(send_buf);
    }

    MPI_Finalize();
    return 0;
}